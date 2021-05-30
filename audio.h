#ifndef _AUDIO_H_
#define _AUDIO_H_

#define PCM_DEVICE_NAME "default"

/* PROTOTYPES */

extern int Audio_open(const char *name);
extern void Audio_close(void);
extern int Audio_write(void *data, unsigned long nr_frames);

extern void Audio_init(const char *name, double volume, double *max_volume);
extern const char *Audio_state(void);

#ifdef HAVE_LIBSOX
extern void Audio_file_play(char *fname, long left_volume,
	long right_volume);
extern void Audio_file_stop(void);
#endif
#endif /* _AUDIO_H_ */
