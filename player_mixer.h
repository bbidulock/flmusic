#ifndef _PLAYER_MIXER_H_
#define _PLAYER_MIXER_H_

extern int Mixer_init_control(const char *card, double volume,
	double *max_volume);
extern void Mixer_change_volume(long left_vol, long right_vol);


#endif /* _PLAYER_MIXER_H_ */
