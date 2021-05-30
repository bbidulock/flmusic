#include <config.h>
/*
 * TODO:
*/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>

#include <linux/cdrom.h>
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
#include <FL/fl_draw.H>
#include <FL/Enumerations.H>
#include <FL/Fl_Widget.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Valuator.H>
#include <FL/Fl_Line_Dial.H>
#include <FL/Fl_Output.H>
#include <FL/Fl_Progress.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Int_Input.H>
#include <FL/Fl_File_Chooser.H>
#include <FL/Fl_Native_File_Chooser.H>
#include <FL/Fl_Multiline_Output.H>
#include <FL/Fl_Box.H>


/* szukw000: linux-2.6.32: ioctl returns '-1', perror 'Input/output error'
 *           Is this drive dependent? Or kernel version dependent ?
*/
/* #define USE_CDROM_PAUSE_RESUME_IOCTL */
#ifdef USE_CDROM_PAUSE_RESUME_IOCTL
#include <sys/ioctl.h>
#endif

#include "audio.h"
#include "reader.h"
#include "common.h"
#include "player_mixer.h"
#include "lang.h_utf8"

#define DEFAULT_INPUT_DEVICE "/dev/cdrom"

#define DEFAULT_MAX_VOLUME 255. /* szukw000: hopefully true */
#define START_VOLUME 0.5

#define SLIDER_DELAY 1.0
//After close_tray: WAIT_FOR_DEVICE seconds
#define WAIT_FOR_DEVICE 8.0

/* FORWARD */
static void send_cdrom_data(void);
static void show_info_cb(Fl_Widget *wid, void *v);
static void resume_cdrom(void);

static int start_disc(int track);
static void stop_disc(void);

/*------------------------------------------*/
#ifdef HAVE_LIBSOX
static char *read_idf;
#endif

static char *sound_url;

static int is_playing_file;
static int is_playing, is_playing_cdrom, is_playing_url;
static unsigned long audio_frames;

static unsigned char audio_buf[BUF_MAXLEN];

static Fl_Button *play_file_button; 
static Fl_Button *pause_player_button;
static Fl_Button *play_cdrom_button;

static Fl_Group *track_group;
static Fl_Box *track_of;
static Fl_Output *track_out, *all_tracks_out;

#ifdef USE_PROGRESS_BAR
static Fl_Progress *elapsed_bar;
#else
static Fl_Output *elapsed_out;
static char pzero[32];
#endif
static char pbuf[32];

static Fl_Group *goto_group;
static Fl_Int_Input *goto_track;
static Fl_Group *change_track_group, *change_frame_group;

static Fl_Button *open_tray_button;
static Fl_Line_Dial *volume_dial, *balance_dial;

static int select_prev_track, select_next_track;
static int select_prev_frame, select_next_frame;
static int selected_track;

static long left_volume, right_volume;
static double max_volume;
static double slider_delay;

static int progress_start = 1;
static int player_stopped, player_muted, open_tray, tray_active;
static int player_pause_active, transfer_must_stop;
static int player_must_stop;

static PlayInfo *play;

static pthread_t player_thread;
/*-----------------------------------------*/

static void *play_cdrom_thread(void *v)
{
	audio_frames = 0;

    while(is_playing)
   {
	if(transfer_must_stop) break;

	send_cdrom_data();

    if(audio_frames == 0) break;

    if(transfer_must_stop) break;

    Audio_write(audio_buf, audio_frames);

    audio_frames = 0;
 
   }/* for() */

	Audio_close();

	if( !player_stopped)
   {
	is_playing = is_playing_cdrom = 0;
   }
	return NULL;
}/* play_cdrom_thread() */

#ifdef HAVE_LIBSOX

static void activate_cdrom(int v)
{
	if(v)
   {
	track_group->activate();
	goto_group->activate();

	change_track_group->activate();
	change_frame_group->activate();
	pause_player_button->activate();
	play_cdrom_button->activate();
	open_tray_button->activate();
	return;
   }	
	track_group->deactivate();
	goto_group->deactivate();

	change_track_group->deactivate();
	change_frame_group->deactivate();
	pause_player_button->deactivate();
	play_cdrom_button->deactivate();
	open_tray_button->deactivate();
}

static void activate_file(int v)
{
	if(v)
	 play_file_button->activate();
	else
	 play_file_button->deactivate();
}

#endif /* HAVE_LIBSOX */

static void clear_progress(void)
{
    all_tracks_out->value("");
	all_tracks_out->redraw();
    track_out->value("");
	track_out->redraw();

	Player_set_progress(-0.01);
}

static void goto_track_cb(Fl_Widget *wid, void *v)
{
	if(wid->contains(Fl::belowmouse()))//Fl::focus()
   {
	Fl_Int_Input *inp;
	const char *cs;

	if( !is_playing) return;

	inp = (Fl_Int_Input*)wid;

	if((cs = inp->value()) == NULL) return;

	int i = atoi(cs);

	if(i < play->list.first_track) i = play->list.first_track;

	if(i > play->list.last_track) i = play->list.last_track;

	selected_track = i;
   }
}

static void prev_track_cb(Fl_Widget *wid, void *v)
{
	select_prev_track = 1;
}

static void next_track_cb(Fl_Widget *wid, void *v)
{
	select_next_track = 1;
}

static void prev_frame_cb(Fl_Widget *wid, void *v)
{
	select_prev_frame = 1;
}

static void next_frame_cb(Fl_Widget *wid, void *v)
{
	select_next_frame = 1;
}

static void stop_transfer(void)
{
    if(is_playing_cdrom)
   {
    transfer_must_stop = 1;

    while(is_playing)
     Fl::wait(0.01);
   
    stop_disc();
   }
	transfer_must_stop = 0;
}

static void tray_cb(Fl_Widget *wid, void *v)
{
	Fl_Button *b = (Fl_Button*)wid;

	if(player_stopped) return;
	if(tray_active) return; 
	tray_active = 1;

	open_tray ^= 1;

	if(open_tray)
   {
	b->label(CLOSE_TRAY_s); b->redraw();
    play_cdrom_button->label(CD_START_s);
	play_cdrom_button->redraw();

	stop_transfer();

	clear_progress();

	Reader_open_tray(DEFAULT_INPUT_DEVICE);
   }
	else
   {
	Reader_close_tray(DEFAULT_INPUT_DEVICE);

	b->label(OPEN_TRAY_s); b->redraw();
    play_cdrom_button->label(CD_START_s);
	play_cdrom_button->redraw();
   }
	tray_active = 0;
}

static void pause_player_cb(Fl_Widget *wid, void *v)
{
	Fl_Button *b = (Fl_Button*)wid;
/* szukw000: implemented only for CDROM drive, not for FILE :
*/
	if( !is_playing_cdrom) return;
	if(player_pause_active) return; 
	player_pause_active = 1;

	player_stopped ^= 1;

	if(player_stopped)
   {//ioctl(cdrom_fd, CDROMPAUSE)
	player_must_stop = 1;
	b->label(RESTART_PLAYER_s);

#ifdef USE_CDROM_PAUSE_RESUME_IOCTL
errno = 0;
	int v = ioctl(play->cdrom_fd, CDROMPAUSE);
perror("CDROMPAUSE");
fprintf(stderr,"%s:%d:PAUSE ioctl(%d)\n",__FILE__,__LINE__,v);
#else
	close(play->cdrom_fd);
	play->cdrom_fd = -1;
#endif
   }
	else
   {
	b->label(PAUSE_PLAYER_s);

#ifdef USE_CDROM_PAUSE_RESUME_IOCTL
errno = 0;
	int v = ioctl(play->cdrom_fd, CDROMRESUME);
perror("CDROMRESUME");
fprintf(stderr,"%s:%d:RESUME ioctl(%d)\n",__FILE__,__LINE__,v);
#else
	play->cdrom_fd = open(DEFAULT_INPUT_DEVICE, O_RDONLY | O_NONBLOCK);
#endif
	if(play->cdrom_fd >= 0)
  {
	resume_cdrom();
  }
   }
	player_pause_active = 0;
}

static void mute_cb(Fl_Widget *wid, void *v)
{
	Fl_Button *b = (Fl_Button*)wid;

	if( !is_playing) return;

	player_muted ^= 1;

	if(player_muted)
   {
	b->label(UNMUTE_s);
	Mixer_change_volume(0, 0);
   }
	else
   {
	b->label(MUTE_s);
	Mixer_change_volume(left_volume, right_volume);
   }
}

static void change_left_and_right_volume(void)
{
	if(!is_playing) return;

    double vol = volume_dial->value() *  max_volume;
    double d = 0.5 - balance_dial->value();

    left_volume = (long)(vol*(1. - d));
    right_volume = (long)(vol*(1 + d));

    Mixer_change_volume(left_volume, right_volume);

}

static void volume_cb(Fl_Widget *wid, void* v) 
{
	change_left_and_right_volume();
}

static void balance_cb(Fl_Widget *wid, void* v) 
{
	change_left_and_right_volume();
}

void Player_set_progress(double v)
{
#ifdef USE_PROGRESS_BAR
	if(elapsed_bar->value() < 100.)
   {
	char buf[16];

	v *= 100.;
	elapsed_bar->value(v);
	sprintf(buf,"%3d%%", (int)v);
	elapsed_bar->copy_label(buf);
   }
	return;
#else
    if((v += 0.01) > 1.0) v = 1.0;
    sprintf(pbuf, "%.2f", v);
    elapsed_out->value(pbuf);

#endif
}

static void progress_timeout_cb(void* v) 
{
	if(play == NULL) 
   {
	return;
   }
	if(play->frame_pos >= play->track_len)
   {
	sprintf(pbuf, "%d", play->cur_track%(play->nr_tracks+1));
	track_out->value(pbuf);

#ifdef USE_PROGRESS_BAR
	elapsed_bar->value(0.);
#else
	elapsed_out->value(pzero);
#endif

	return;
   }
	Player_set_progress((double)play->frame_pos/(double)play->track_len);

	Fl::repeat_timeout(slider_delay, progress_timeout_cb, v);
}

static void progress()
{
#ifdef USE_PROGRESS_BAR
	elapsed_bar->maximum(100.0);
	elapsed_bar->value(0.);
#else
	elapsed_out->value(pzero);
#endif
	sprintf(pbuf, "%d", play->cur_track);
	track_out->value(pbuf);
	track_out->redraw();

	sprintf(pbuf, "%d", play->nr_tracks);
	all_tracks_out->value(pbuf);
	all_tracks_out->redraw();

	slider_delay = SLIDER_DELAY;

	if(progress_start)
   {
	progress_start = 0;
	Fl::add_timeout(slider_delay, progress_timeout_cb, play);
   }
	else
	 Fl::repeat_timeout(slider_delay, progress_timeout_cb, play);
}

#ifdef HAVE_LIBSOX

static int is_file(const char *cs)
{
    FILE *fp;

    if((fp = fopen(cs, "rb")) == NULL) return 0;

    fclose(fp); return 1;
}

static void file_finished(void)
{
    play_file_button->label(FILE_START_s);
    play_file_button->redraw();
    clear_progress();
    activate_cdrom(1);
    is_playing = is_playing_file = 0;
}

static void *play_file_thread(void *v)
{
#ifdef USE_PROGRESS_BAR
	elapsed_bar->maximum(100.);
	elapsed_bar->value(0.);
#else
	elapsed_out->value(pzero);
#endif
	Audio_file_play((char*)v, left_volume, right_volume);

	free(v);

	file_finished();

	return NULL;
}/* play_file_thread() */

static void play_sound_file(const char *fname)
{
	activate_cdrom(0);

    pthread_create(&player_thread, NULL, &play_file_thread, strdup(fname));
}

static void file_chooser(const char *cs)
{
	if( !is_playing)
   {
	is_playing = is_playing_file = 1;
	play_file_button->label(FILE_STOP_s);

	if( !is_file(cs))
  {
	is_playing = is_playing_file = 0;
	play_file_button->label(FILE_START_s);
	play_file_button->redraw();

	return;
  }
	play_file_button->label(FILE_STOP_s);

	if(read_idf) free(read_idf);
	read_idf = strdup(cs);

	play_sound_file(cs);

	return;
   }
	Audio_file_stop();
	file_finished();

}//file_chooser()

static void browse_cb(Fl_Widget *wid, void *v)
{
	if(open_tray) return;
	if(player_stopped) return;

	if(is_playing && is_playing_file)
   {
	Audio_file_stop();

	file_finished();

	return;
   }
   {//block
	Fl_Native_File_Chooser native;
	native.title(SELECT_SOUNDFILE_s);
	native.type(Fl_Native_File_Chooser::BROWSE_SAVE_FILE);
	native.filter("*.*");
	native.preset_file(read_idf);

	switch(native.show())
  {
	case -1:
		fprintf(stderr, "FILE_CHOOSER_ERROR: %s\n", native.errmsg());
		break;

	case  1: /* Cancel */
		break;

	default:
		if(open_tray) break;
		if(player_stopped) break;

		if(is_playing && is_playing_file)
	   {
		Audio_file_stop();

		file_finished();

		break;
	   }
		if(native.filename())
	   {
		file_chooser(native.filename());
	   }
		break;
  }	//switch()
   }//block
}

#endif /* HAVE_LIBSOX */

static void resume_cdrom(void)
{
	if( !Audio_open(PCM_DEVICE_NAME))
   {
	fl_alert("resume_cdrom: can not open audio device");
	return;
   }
	is_playing_cdrom = is_playing = 1;

    pthread_create(&player_thread, NULL, &play_cdrom_thread, NULL);
}

static void play_cdrom(int track_nr)
{

	if( !Audio_open(PCM_DEVICE_NAME))
   {
	fl_alert("play_cdrom(): can not open audio device");
	return;
   }
	is_playing_cdrom = is_playing = 1;

	if(play == NULL)
   {
	if(start_disc(track_nr) == 0)
  {
	is_playing_cdrom = is_playing = 0;
	Audio_close();

	return;
  }
   }
	play_cdrom_button->label(CD_STOP_s);
#ifdef HAVE_LIBSOX
	activate_file(0);
#endif
	progress_start = 1;
	progress();
	Mixer_change_volume(left_volume, right_volume);

    pthread_create(&player_thread, NULL, &play_cdrom_thread, NULL);
}

static void play_cdrom_cb(Fl_Widget *wid, void *v)
{
	if(player_stopped) return;
	if(is_playing_file) return;
	if( !Reader_disk_ok()) 
   {
	fl_message("NO_DISK in tray.");
	return;
   }
	if(is_playing)
   {
	play_cdrom_button->label(CD_START_s);

	stop_transfer();

	clear_progress();

#ifdef HAVE_LIBSOX
	activate_file(1);
#endif

	return;	
   }
	Fl::wait(WAIT_FOR_DEVICE);

	play_cdrom(1);
}

static void exit_cb(Fl_Widget *wid, void *v)
{
	stop_transfer();

#ifdef HAVE_LIBSOX
	if(is_playing_file)
	 Audio_file_stop();

	if(read_idf) free(read_idf);
#endif

	exit(0);
}

static void print_help(const char *cs)
{
#ifdef WITH_ENGLISH
    fprintf(stderr,"\n'%s' knows the following options:"
    "\n    %s --track=NR\n    %s --file=NAME"
    "\n    %s\n\n",PACKAGE_STRING,cs,cs,cs);
#endif
#ifdef WITH_GERMAN
    fprintf(stderr,"\n'%s' kennt folgende Optionen:"
    "\n    %s --track=NUMMER\n    %s --file=NAME"
    "\n    %s\n\n",PACKAGE_STRING,cs,cs,cs);
#endif
}

static void print_version(const char *cs)
{
	char sver[32];
#ifdef HAVE_LIBSOX
	snprintf(sver,31,"(libsox version %s)", sox_version());
#else
	strcpy(sver,"(without libsox)");
#endif /* HAVE_LIBSOX */

#ifdef WITH_ENGLISH
    fprintf(stderr,"\n'%s'%s\n\n", PACKAGE_STRING, sver);
#endif
#ifdef WITH_GERMAN
    fprintf(stderr,"\n'%s'%s\n\n", PACKAGE_STRING, sver);
#endif

}

int main(int argc, char *argv[]) 
{
#ifdef HAVE_LIBSOX
    const char *sound_file = NULL;
	int shall_play_file = 0;
#endif
    int shall_play_cdrom = 0, shall_play_url = 0;
    int track_nr = 1;
	int w = 550, h = 230;
/*-----------------------------------------*/
	is_playing_url = 0;
    if(argc == 2)
   {
    if(strcmp(argv[1], "--help") == 0
    || strcmp(argv[1], "-help") == 0
    || strcmp(argv[1], "-h") == 0
    || strcmp(argv[1], "?") == 0)
  {
    print_help(argv[0]);
    return 0;
  }
    if(strcmp(argv[1], "--version") == 0
    || strcmp(argv[1], "-version") == 0)
  {
    print_version(argv[0]);
    return 0;
  }
    if(strncmp(argv[1], "--track=", 8) == 0)
  {
    track_nr = atoi(argv[1] + 8);
    shall_play_cdrom = 1;
  }
#ifdef HAVE_LIBSOX
    else
    if(strncmp(argv[1], "--file=", 7) == 0)
  {
    sound_file = argv[1] + 7;

    if((shall_play_file = is_file(sound_file)) == 0)
     sound_file = NULL;
  }
#endif /* HAVE_LIBSOX */
    else
    if(strncmp(argv[1], "--url=", 6) == 0)
  {
    sound_url = NULL; shall_play_url = 0;
#ifdef UNUSED_CODE
    sound_url = argv[1] + 6;
    if((shall_play_url = is_url(sound_url)) == 0)
     sound_url = NULL;
#endif
    fprintf(stderr,"playing URL not implemented yet.\n");
    return 0;
  }
    else
  {
    shall_play_cdrom = 1; track_nr = 1;
  }
   }
/*------------------------*/
	remove(PLAYINFO_FILE);
/*------------------------*/
	Fl_Window *main_win = new Fl_Window(w, h, PLAYER_TITLE_s);
	main_win->begin();
	main_win->box(FL_FLAT_BOX);

	int x = 55, y = 20;
	track_group = new Fl_Group(5,y-2,155+4,25+4);
	track_group->begin();

	track_out = new Fl_Output(x,y,30,25,TRACK_s);
	track_out->align(FL_ALIGN_LEFT);
	track_out->box(FL_THIN_DOWN_BOX);
	track_out->color(FL_WHITE);
	track_out->selection_color(FL_WHITE);
	track_out->value("");
	track_out->clear_visible_focus();

	x += 35;
	track_of = new Fl_Box(x,y,30,25, OF_s);
	track_of->color(main_win->color());
	track_of->box(FL_FLAT_BOX);

	x += 35;
	all_tracks_out = new Fl_Output(x,y,30,25);
	all_tracks_out->box(FL_THIN_DOWN_BOX);
	all_tracks_out->color(FL_WHITE);
	all_tracks_out->selection_color(FL_WHITE);
	all_tracks_out->value("");
	all_tracks_out->clear_visible_focus();

	track_group->end();
	x = track_group->x() + track_group->w() + 5;
#ifdef USE_PROGRESS_BAR
	x += 35;
	elapsed_bar = new Fl_Progress(x,y,w-x-10,25);
	elapsed_bar->box(FL_THIN_DOWN_BOX);
	elapsed_bar->selection_color(fl_rgb_color(238, 221, 130) );
	elapsed_bar->labelcolor(FL_BLACK);
	elapsed_bar->labelfont(FL_HELVETICA);
	elapsed_bar->maximum(100.0);
#else
	x += 115;
	elapsed_out = new Fl_Output(x,y,40,25, ELAPSED_s);
  	elapsed_out->align(FL_ALIGN_LEFT);
	elapsed_out->selection_color(FL_WHITE);
	elapsed_out->color(FL_WHITE);
	elapsed_out->value("");
	elapsed_out->clear_visible_focus();
	sprintf(pzero, "%.2f", 0.);
#endif
/*-----------------------------------------------------------*/
	x = 125; y = 55;
	goto_group = new Fl_Group(5, y, 151, 32);
	goto_group->begin();

	goto_track = new Fl_Int_Input(x, y+2, 30, 25);
	goto_track->label(GOTO_TRACK_s);
	goto_track->box(FL_THIN_DOWN_BOX);
	goto_track->callback(goto_track_cb);
	goto_track->when(FL_WHEN_ENTER_KEY_ALWAYS);

	goto_group->end();

	x = goto_group->x() + goto_group->w() + 5; 
	int gw = (w - 20 - x)/2; int ww = gw - 44;
   {
	Fl_Group *g = new Fl_Group(x, y, gw, 29);
	g->box(FL_EMBOSSED_BOX);
	g->begin();

	int gx = x+2, gy = y+2;

	Fl_Button *prev_track = new Fl_Button(gx, gy, 20, 25, "@<");
	prev_track->callback(prev_track_cb);

	gx += 20;
	Fl_Box *l = new Fl_Box(gx, gy, ww, 25, CHANGE_TRACK_s);
	l->box(FL_FLAT_BOX);
	l->color(main_win->color());

	gx += ww;
	Fl_Button *next_track = new Fl_Button(gx, gy, 20, 25, "@>");
	next_track->callback(next_track_cb);

	g->end();
	g->resizable(NULL);
	change_track_group = g;
   }
	x += gw + 10;
   {
	Fl_Group *g = new Fl_Group(x, y, gw, 29);
	g->box(FL_EMBOSSED_BOX);
	g->begin();

	int gx = x+2, gy = y+2;

	Fl_Button *prev_frame = new Fl_Button(gx, gy, 20, 25, "@<");
	prev_frame->callback(prev_frame_cb);

	gx += 20;
	Fl_Box *l = new Fl_Box(gx, gy, ww, 25, CHANGE_FRAME_s);
	l->box(FL_FLAT_BOX);
	l->color(main_win->color());

	gx += ww;
	Fl_Button *next_frame = new Fl_Button(gx, gy, 20, 25, "@>");
	next_frame->callback(next_frame_cb);

	g->end();
	g->resizable(NULL);
	change_frame_group = g;
   }
/*-----------------------------------------------------------*/
	x = 10; y = h-135;
   {
	Fl_Group *g = new Fl_Group(x,y,60,60+15);
	g->begin();

	volume_dial  = new Fl_Line_Dial(x, y, 60, 60, VOLUME_s);
	volume_dial->labelsize(12);
	volume_dial->color((Fl_Color)10);
	volume_dial->selection_color((Fl_Color)1);
	volume_dial->value(START_VOLUME);
	volume_dial->callback(volume_cb);
	volume_dial->angle1(5); volume_dial->angle2(355);

	g->end();
   }   
	x += 75;
   {
    Fl_Group *g = new Fl_Group(x,y,60,60+15);
    g->begin();

	balance_dial  = new Fl_Line_Dial(x, y, 60, 60, BALANCE_s);
	balance_dial->labelsize(12);
	balance_dial->color((Fl_Color)10);
	balance_dial->selection_color((Fl_Color)1);
	balance_dial->value(0.5);
	balance_dial->callback(balance_cb);
	balance_dial->angle1(5); balance_dial->angle2(355);

	g->end();
   }
	x = w - 50;
   {
	Fl_Button *b = new Fl_Button(x,y,40,25,"Info");
	b->callback(show_info_cb);
   }
	x -= 120;
   {
	Fl_Button *b = new Fl_Button(x,y,110,25);
	b->label(MUTE_s);
	b->callback(mute_cb);
	player_muted = 0;
   }
   {
	pause_player_button = new Fl_Button(x, y+30,110,25);
	pause_player_button->label(PAUSE_PLAYER_s);
	pause_player_button->callback(pause_player_cb);
   }
/*-----------------------------------------------------------*/
	y = h-35; int bw = (w-40)/4; x = 5;
   {
	play_cdrom_button = new Fl_Button(x, y, bw-10, 25, CD_START_s);
	play_cdrom_button->callback(play_cdrom_cb);
   }
	x += bw;
   {
	open_tray_button = new Fl_Button(x,y,bw+10,25, OPEN_TRAY_s);
	open_tray_button->callback(tray_cb);
   }
	x += bw + 20;
   {
	play_file_button = new Fl_Button(x,y,bw+10,25, FILE_START_s);
#ifdef HAVE_LIBSOX
	play_file_button->callback(browse_cb);
#else
	play_file_button->deactivate();
#endif
   }
	x += bw + 20;
   {
	Fl_Button *b = new Fl_Button(x, y, bw-10, 25, EXIT_s);
	b->callback(exit_cb);
   }

	main_win->end();
	main_win->show();

	max_volume = 0.;

	Audio_init(PCM_DEVICE_NAME, volume_dial->value(), &max_volume);

	if(max_volume == 0.) max_volume = DEFAULT_MAX_VOLUME;

	left_volume = right_volume = (long)(volume_dial->value() * max_volume);

	if(shall_play_cdrom)
   {
	play_cdrom(track_nr);
   }
#ifdef HAVE_LIBSOX
	else
	if(shall_play_file)
   {
	is_playing = is_playing_file = 1;
	play_file_button->label(FILE_STOP_s);

	play_sound_file(sound_file);
   }
#endif

	while(main_win->visible())
   {
	Fl::wait();
   }
	return 0;
}//main()

static int read_track(void)
{
	int nr;
	audio_frames = 0;

	if(play->track_len == 0
	|| play->frame_pos >= play->track_len)
   {
	return 0;
   }
	nr = play->track_len - play->frame_pos;

	if(nr > NR_CDFRAMES) nr = NR_CDFRAMES;

	if( !Reader_read_cd(play->cdrom_fd,
		play->track_start + play->frame_pos, 
		nr, audio_buf))
   {
	return 0;
   }
	play->frame_pos += nr;
/*	audio_frames = (unsigned long)(BUF_MAXLEN>>2); */
	audio_frames = (unsigned long)(CD_FRAMESIZE_RAW * nr)>>2;

	return 1;

}/* read_track() */


static int start_disc(int track)
{
	if((play = Reader_open(DEFAULT_INPUT_DEVICE)))
   {
	play->nr_tracks = play->list.last_track;

	if(track) play->start_track = track;

	if(play->start_track < play->list.first_track)
	 play->start_track = play->list.first_track;

	if(play->start_track > play->nr_tracks)
	 play->start_track = play->nr_tracks;

	play->cur_track = play->start_track;

	play->track_start = play->list.track_start[play->cur_track-1];
	play->track_len = play->list.track_start[play->cur_track] -
    	play->list.track_start[play->cur_track-1];

	return 1;
   }
	return 0;
}

static void read_next_track(void)
{
	int i;

	if((i = play->cur_track) < play->nr_tracks) 
   {
	play->cur_track = ++i;
	play->frame_pos = 0;
	play->track_start = play->list.track_start[i-1];
	play->track_len = 
	 play->list.track_start[i] - play->list.track_start[i-1];

	if(read_track())
  {
	progress();
	return;
  }
   }
	 stop_disc();
}

static void send_cdrom_data(void)
{
	if(transfer_must_stop) return;

	if(player_must_stop)
   {
	player_must_stop = 0;
	return;
   }

	if(select_prev_track)
   {
	if(play->cur_track > play->list.first_track)
  {
	selected_track = play->cur_track - 1;
	select_prev_track = 0;
  }
   }
	else
	if(select_next_track)
   {
	if(play->cur_track < play->list.last_track)
  {
	selected_track = play->cur_track + 1;
	select_next_track = 0;
  }
   }
	else
	if(select_prev_frame)
   {
	int step = NR_CDFRAMES<<6;

	if(play->frame_pos > step) 
	 play->frame_pos -= step;
	select_prev_frame = 0;
   }
	else
	if(select_next_frame)
   {
	int step = NR_CDFRAMES<<6;

	if(play->frame_pos < play->track_len - step)
	 play->frame_pos += step;

	select_next_frame = 0;
   }

	if(selected_track)
   {
	play->cur_track = selected_track - 1;
	selected_track = 0;
/*==================================*/
	read_next_track();
/*==================================*/
   }
/*==================================*/
	if(read_track()) return;
/*==================================*/
	if(transfer_must_stop) return;

	if(selected_track)
   {
	play->cur_track = selected_track - 1;
	selected_track = 0;
   }
/*==================================*/
	read_next_track();
/*==================================*/
}

static void stop_disc(void)
{
	Reader_close(play);

	play = NULL; is_playing_cdrom = is_playing = 0;

	play_cdrom_button->label(CD_START_s);
	play_cdrom_button->redraw();
}

static void info_exit_cb(Fl_Widget *wid, void *v)
{
	delete (Fl_Window*)v;
}

static void show_info_cb(Fl_Widget *wid, void *v)
{
	FILE *fp;
	char *buf;
	int w, h;
	struct stat sb;
	w = 300; h = 200;

	if(stat(PLAYINFO_FILE, &sb) < 0
	|| sb.st_size == 0) return;

	buf = (char*)malloc(sb.st_size + 4);
	fp = fopen(PLAYINFO_FILE, "r");
	fread(buf, 1, sb.st_size, fp);
	fclose(fp);
	buf[sb.st_size] = 0;

	Fl_Window *win = new Fl_Window(w, h);
	win->begin();

	Fl_Multiline_Output *ml = new Fl_Multiline_Output(5,5,w-10,h-40);
	ml->value(buf);
	ml->textfont(FL_COURIER);
	ml->position(0,0);
	free(buf);
	win->resizable(ml);

	Fl_Button *b = new Fl_Button(w/2 - 40,h-30,80,25, "OK");
	b->callback(info_exit_cb, win);

	win->end();
	win->show();
	while(win->visible())
	 Fl::wait();
}
