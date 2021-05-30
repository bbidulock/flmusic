#include <config.h>
#include <stdio.h>
#include <errno.h>
#include <assert.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <alsa/asoundlib.h>
#ifdef HAVE_LIBSOX
#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#include <sox.h>
           }
#else
#include <sox.h>
#endif /* __cplusplus */
#endif

#include <FL/Fl.H>
#include <FL/fl_ask.H>

#include "common.h"
#include "player_mixer.h"
#include "file_info.h"
#include "reader.h"
#include "audio.h"


static snd_pcm_t *sound_handle;
static snd_output_t *errlog;

const char *Audio_state(void)
{
	snd_pcm_state_t state = snd_pcm_state(sound_handle);
	return snd_pcm_state_name(state);
}

void Audio_init(const char *name, double volume, double *max_volume)
{
    snd_pcm_open(&sound_handle, name, SND_PCM_STREAM_PLAYBACK,0);

	if(sound_handle)
   {
	Mixer_init_control(name, volume, max_volume);

	snd_pcm_close(sound_handle);
	sound_handle = NULL;
   }
}

int Audio_open(const char *name)
{
	snd_pcm_hw_params_t *hwparams;
	snd_pcm_sw_params_t *sw_params;
	snd_pcm_uframes_t boundary = 0;
    snd_pcm_uframes_t frames, min_frames, max_frames, buf_len;
	snd_pcm_uframes_t period;
    unsigned int n;

	int err;

	if(!name || !*name) 
   {
	name = PCM_DEVICE_NAME;
   }	
/* The last argument sets the open mode:
 * zero seems to be the default: BLOCK until resources are free 
 * SND_PCM_NONBLOCK 1
 * SND_PCM_ASYNC    2
*/
	err = snd_pcm_open(&sound_handle, name, SND_PCM_STREAM_PLAYBACK, 0); 

	if(err < 0)
   {
	fl_alert("Audio_open: open error with name(%s)\n%s\n",name,
	 snd_strerror(err));
	return 0;
   }
	err = 
	snd_output_stdio_attach(&errlog, stderr, 0);
	if(err < 0) goto fails;

    err =
    snd_pcm_hw_params_malloc(&hwparams);
	if(err < 0) goto fails;

    err =
    snd_pcm_hw_params_any(sound_handle, hwparams);
	if(err < 0) goto fails;

    err =
    snd_pcm_hw_params_set_rate_resample(sound_handle, hwparams, 1);
	if(err < 0) goto fails;

    err =
    snd_pcm_hw_params_set_access(sound_handle, hwparams,
        SND_PCM_ACCESS_RW_INTERLEAVED);
	if(err < 0) goto fails;

    err =
    snd_pcm_hw_params_set_format(sound_handle, hwparams,
//		SND_PCM_FORMAT_S16);
		SND_PCM_FORMAT_S16_LE);
	if(err < 0) goto fails;

    n = 44100;//(44100 | 48000 ).
/* last argument is 'dir': 
 *       0 means exactly, 
 *      -1 means less than, 
 *       1 means greater than
*/
    err =
    snd_pcm_hw_params_set_rate_near(sound_handle, hwparams, &n, 0);
	if(err < 0) goto fails;

    n = 2;

    err =
    snd_pcm_hw_params_set_channels_near(sound_handle, hwparams, &n);
	if(err < 0) goto fails;

/* BUF_MAXLEN = CD_FRAMESIZE_RAW * NR_CDFRAMES */
    frames = BUF_MAXLEN / 2 / n; 
	min_frames = max_frames = 0;

    err =
    snd_pcm_hw_params_get_buffer_size_min(hwparams, &min_frames);
	if(err < 0) goto fails;

    err =
    snd_pcm_hw_params_get_buffer_size_max(hwparams, &max_frames);
	if(err < 0) goto fails;

    if(frames < min_frames) frames = min_frames;
    if(frames > max_frames) frames = max_frames;

	period = frames/2;
    buf_len = frames * 2;

/* pcm.c : approximate target buffer size in frames */
    err =
    snd_pcm_hw_params_set_buffer_size_near(sound_handle, hwparams, 
		&frames);
	if(err < 0) goto fails;

    err =
    snd_pcm_hw_params(sound_handle, hwparams);
	if(err < 0) goto fails;

    snd_pcm_hw_params_free(hwparams); 

	err =
	snd_pcm_sw_params_malloc(&sw_params);

	err =
	snd_pcm_sw_params_current(sound_handle, sw_params);

	err =
	snd_pcm_sw_params_set_avail_min(sound_handle, sw_params, 
		buf_len/4);

	err =
	snd_pcm_sw_params_set_start_threshold(sound_handle, sw_params, 0);

	err =
	snd_pcm_sw_params_get_boundary(sw_params, &boundary);

	err =
	snd_pcm_sw_params_set_stop_threshold(sound_handle, sw_params, boundary);

	err =
	snd_pcm_sw_params_set_silence_threshold(sound_handle, sw_params, 0);

	err =
	snd_pcm_sw_params(sound_handle, sw_params);

	snd_pcm_sw_params_free(sw_params);

    err =
    snd_pcm_prepare(sound_handle);
	if(err < 0) goto fails;

/*	snd_pcm_start(sound_handle); */
	return 1;

fails:
	if(hwparams)
	 snd_pcm_hw_params_free(hwparams);

	fl_alert("Audio_open fails with\n=%s=\n",snd_strerror(err));
	return 0;
}/* Audio_open() */

void Audio_close(void)
{
	if(sound_handle) 
   {
	snd_pcm_drain(sound_handle);
	snd_pcm_close(sound_handle);
	sound_handle = NULL;
   }	
	if(errlog)
   {
	snd_output_close(errlog);
   }
	return;
}

static int xrun_recovery(snd_pcm_t *handle, int err)
{
	if(err == -EPIPE) 
   {    
/* under-run */
	err = snd_pcm_prepare(handle);
	if(err < 0)
  {
	fl_alert("xrun_recovery: cannot recover from underrun\nprepare failed:"
	" %s\n", snd_strerror(err));
  }
	return 0;
   } 

	if(err == -ESTRPIPE) 
   {
	while ((err = snd_pcm_resume(handle)) == -EAGAIN)
		sleep(1);
	if(err < 0) 
  {
	err = snd_pcm_prepare(handle);
	if(err < 0)
 {
	fl_alert("xrun_recovery: cannot recover from suspend, prepare failed:"
	" %s\n", snd_strerror(err));
 }
  }
	return 0;
   }
	return err;
}

int Audio_write(void *data, unsigned long nr_frames)
{
	int err;

	if(!sound_handle) 
   {
	fl_alert("Audio_write: sound_handle missing.\n");
	return 0;
	}	
	err = snd_pcm_writei(sound_handle, data, nr_frames);

	if(err < 0) 
   {
	if(xrun_recovery(sound_handle, err) < 0) 
  {
	fl_alert("Audio_write: xrun_recovery error I\n");
	return 0;
  }	
	err = snd_pcm_writei(sound_handle, data, nr_frames);
	if(err < 0) 
  {
	if(xrun_recovery(sound_handle, err) < 0) 
 {
	fl_alert("Audio_write: xrun_recovery error II\n");
	return 0;
 }	
  }	
   }
	return 1;
}

#ifdef HAVE_LIBSOX

static sox_format_t *sox_out, *sox_in;
static int file_stopped;

static void file_progress_timeout_cb(void* v)
{
	double tv = (double)sox_in->olength/(double)sox_in->signal.length;

	Player_set_progress(tv);

    Fl::repeat_timeout(1.0, file_progress_timeout_cb, v);
}

#if SOX_LIB_VERSION_CODE >= SOX_LIB_VERSION(14,3,0)
static int stop_cb(sox_bool all_done, void *v)
{
	if(file_stopped) return SOX_EOF; return SOX_SUCCESS;
}
#else
static int stop_cb(sox_bool all_done)
{
	if(file_stopped) return SOX_EOF; return SOX_SUCCESS;
}
#endif

void Audio_file_play(char *fname, long left_volume,
	long right_volume)
{
    sox_effect_t *e;
    char * args[10];
	sox_effects_chain_t *chain;

#if SOX_LIB_VERSION_CODE >= SOX_LIB_VERSION(14,3,0)
    if(sox_init() != SOX_SUCCESS)
   {
    fl_alert("sox_init() failed.");
    return;
   }
#else
    if(sox_format_init() != SOX_SUCCESS)
   {
    fl_alert("sox_init() failed.");
    return;
   }
#endif
	file_stopped = 0;

/* sox_open_read(path,signal,encoding,filetype); */
	sox_in =
	sox_open_read(fname, NULL, NULL, NULL);

	if(sox_in == NULL)
   {
	fl_alert("Unusable file found\n%s", fname);
	return;
   }

/* sox_open_write(path,signal,encoding,filetype,overwrite_permitted_func); */

    assert(sox_out =
     sox_open_write("default", &sox_in->signal, NULL, "alsa", NULL, NULL));

	Mixer_change_volume(left_volume, right_volume);

	FILE *fp = fopen(PLAYINFO_FILE, "w");

	FileInfo_write(sox_in, fp);

	fclose(fp);

    chain =
     sox_create_effects_chain(&sox_in->encoding, &sox_out->encoding);

    e =
     sox_create_effect(sox_find_effect("input"));
    args[0] = (char *)sox_in;
    assert(sox_effect_options(e, 1, args) == SOX_SUCCESS);
    assert(sox_add_effect(chain, e, &sox_in->signal, &sox_in->signal)
        == SOX_SUCCESS);

    if (sox_in->signal.rate != sox_out->signal.rate)
   {
    e =
     sox_create_effect(sox_find_effect("rate"));
    assert(sox_effect_options(e, 0, NULL) == SOX_SUCCESS);
    assert(sox_add_effect(chain, e, &sox_in->signal, &sox_out->signal)
        == SOX_SUCCESS);
   }
    e =
     sox_create_effect(sox_find_effect("output"));
    args[0] = (char *)sox_out;
    assert(sox_effect_options(e, 1, args) == SOX_SUCCESS);
    assert(sox_add_effect(chain, e, &sox_in->signal, &sox_out->signal)
        == SOX_SUCCESS);

	Fl::add_timeout(1.0, file_progress_timeout_cb, sox_out);
/*====================================*/
#if SOX_LIB_VERSION_CODE >= SOX_LIB_VERSION(14,3,0)
    sox_flow_effects(chain, stop_cb, NULL);
#else
    sox_flow_effects(chain, stop_cb);
#endif
/*====================================*/
    sox_delete_effects_chain(chain);
	chain = NULL;
	Audio_file_stop();
    sox_close(sox_out);
    sox_close(sox_in);
#if SOX_LIB_VERSION_CODE >= SOX_LIB_VERSION(14,3,0)
    sox_quit();
#else
    sox_format_quit();
#endif
}

void Audio_file_stop(void)
{
	Fl::remove_timeout(file_progress_timeout_cb, sox_out);
	file_stopped = 1;
}

#endif /* HAVE_LIBSOX */
