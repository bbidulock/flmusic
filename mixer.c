#include <config.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <alsa/asoundlib.h>

#include "audio.h"
#include "player_mixer.h"

#ifdef INFORMATION_ONLY
/*-----------------------------------------------------------
alsa-lib-1.0.22/modules/mixer/simple/ac97.c :
 "Master","Master Playback Volume","Master Playback Switch"

alsa-lib-1.0.22/src/mixer/simple_none.c :
 "3D Control - Switch"

Output of 'amixer contents' for CMI-8768:

numid=1,iface=MIXER,name='Master Playback Volume'
  ; type=INTEGER,access=rw------,values=2,min=0,max=31,step=0
  : values=21,21
numid=2,iface=MIXER,name='3D Control - Switch'
  ; type=BOOLEAN,access=rw------,values=1
  : values=on
numid=3,iface=MIXER,name='PCM Playback Switch'
  ; type=BOOLEAN,access=rw------,values=1
  : values=on
numid=41,iface=MIXER,name='PCM Playback Volume'
  ; type=INTEGER,access=rw---RW-,values=2,min=0,max=255,step=0
  : values=167,167
  | dBscale-min=-51.00dB,step=0.20dB,mute=0
(...)
numid=27,iface=MIXER,name='Four Channel Mode'
  ; type=BOOLEAN,access=rw------,values=1
  : values=on
numid=36,iface=PCM,name='IEC958 Playback Con Mask',device=2
  ; type=IEC958,access=r-------,values=1
  : values=[AES0=0xff AES1=0xff AES2=0xff AES3=0xff]
numid=35,iface=PCM,name='IEC958 Playback Default',device=2
  ; type=IEC958,access=rw------,values=1
  : values=[AES0=0x00 AES1=0x82 AES2=0x00 AES3=0x02]
-----------------------------------------------------------*/
#endif /* INFORMATION_ONLY */

int Mixer_init_control(const char *name, double volume, double *max_volume)
{
    static snd_ctl_t *ctl_handle = NULL;
    snd_ctl_elem_info_t *elem_info;
    snd_ctl_elem_id_t *elem_id;
    snd_ctl_elem_value_t *elem_value;
	int err, val;

	err =
	snd_ctl_open(&ctl_handle, name, 0);

	if(err < 0)
   {
	fprintf(stderr,"%s:%d:Mixer_init_control\n\tcan not open %s\n",
	__FILE__,__LINE__,name);

	return 0;
   }
/*-----------*/
    snd_ctl_elem_info_alloca(&elem_info);
    snd_ctl_elem_id_alloca(&elem_id);
    snd_ctl_elem_value_alloca(&elem_value);

	snd_ctl_elem_id_set_interface(elem_id, SND_CTL_ELEM_IFACE_MIXER);
	snd_ctl_elem_id_set_name(elem_id, "PCM Playback Switch");

	snd_ctl_elem_info_set_id(elem_info, elem_id);
	err =
	snd_ctl_elem_info(ctl_handle, elem_info);

	if(err >= 0)
   {
	snd_ctl_elem_info_get_id(elem_info, elem_id);
	snd_ctl_elem_value_set_id(elem_value, elem_id);

	snd_ctl_elem_value_set_boolean(elem_value, 0, 1);

	err =
	snd_ctl_elem_write(ctl_handle, elem_value);
   }
/*-----------*/
    snd_ctl_elem_info_alloca(&elem_info);
    snd_ctl_elem_id_alloca(&elem_id);
    snd_ctl_elem_value_alloca(&elem_value);

	snd_ctl_elem_id_set_interface(elem_id, SND_CTL_ELEM_IFACE_MIXER);
	snd_ctl_elem_id_set_name(elem_id, "3D Control - Switch");

	snd_ctl_elem_info_set_id(elem_info, elem_id);
	err =
	snd_ctl_elem_info(ctl_handle, elem_info);

	if(err >= 0)
   {
	snd_ctl_elem_info_get_id(elem_info, elem_id);
	snd_ctl_elem_value_set_id(elem_value, elem_id);

	snd_ctl_elem_value_set_boolean(elem_value, 0, 1);

	err =
	snd_ctl_elem_write(ctl_handle, elem_value);
   }
/*-----------*/
    snd_ctl_elem_info_alloca(&elem_info);
    snd_ctl_elem_id_alloca(&elem_id);
    snd_ctl_elem_value_alloca(&elem_value);

	snd_ctl_elem_id_set_interface(elem_id, SND_CTL_ELEM_IFACE_MIXER);
	snd_ctl_elem_id_set_name(elem_id, "PCM Playback Volume");

	snd_ctl_elem_info_set_id(elem_info, elem_id);
	err =
	snd_ctl_elem_info(ctl_handle, elem_info);

	if(err >= 0)
   {
	snd_ctl_elem_info_get_id(elem_info, elem_id);
	snd_ctl_elem_value_set_id(elem_value, elem_id);

	val = snd_ctl_elem_info_get_max(elem_info);
	*max_volume = (double)val;

	val = (int)((double)val * volume);
	snd_ctl_elem_value_set_integer(elem_value, 0, val);
	snd_ctl_elem_value_set_integer(elem_value, 1, val);

	err =
	snd_ctl_elem_write(ctl_handle, elem_value);
   }
/*-----------*/
	snd_ctl_close(ctl_handle);

	return 1;

}/* Mixer_init_control() */

void Mixer_change_volume(long left_vol, long right_vol)
{
	snd_mixer_t *mixer_handle;
	snd_mixer_elem_t *elem;
	const char *mixer_name;
	int err, found = 0;

	if((err = snd_mixer_open(&mixer_handle, 0)) < 0)
   {
	fprintf(stderr,"change_volume\nunable to open mixer\n%s",
		snd_strerror(err));
	return;
   }

	err = snd_mixer_attach(mixer_handle, PCM_DEVICE_NAME);
	if(err < 0)
	 fprintf(stderr,"snd_mixer_attach(default):%s\n",snd_strerror(err));

	err = snd_mixer_selem_register(mixer_handle, NULL, NULL);
	if(err < 0)
	 fprintf(stderr,"snd_mixer_selem_register:%s\n",snd_strerror(err));

	if((err = snd_mixer_load(mixer_handle)) < 0)
	 fprintf(stderr,"snd_mixer_load\n%s",snd_strerror(err));
/*======================*/
	mixer_name = "PCM";
/*======================*/
	found = 0;

	for(elem = snd_mixer_first_elem(mixer_handle);
		elem;
		elem = snd_mixer_elem_next(elem))
   {
    if((snd_mixer_elem_get_type(elem) != SND_MIXER_ELEM_SIMPLE)
	|| !snd_mixer_selem_is_active(elem))
      continue;

	if(strcmp((snd_mixer_selem_get_name(elem)), mixer_name)) continue;

	found = 1; break;
   }

	if(found)
   {
	int sval;
/* MONO is an alias for FRONT_LEFT */
	snd_mixer_selem_get_playback_switch(elem, SND_MIXER_SCHN_FRONT_LEFT,&sval);

	if( !snd_mixer_selem_has_playback_switch(elem))
  {
	snd_mixer_selem_set_playback_switch(elem, SND_MIXER_SCHN_FRONT_LEFT, 1);
	snd_mixer_selem_set_playback_switch(elem, SND_MIXER_SCHN_FRONT_RIGHT, 1);
	snd_mixer_selem_set_playback_switch(elem, SND_MIXER_SCHN_REAR_LEFT, 1);
	snd_mixer_selem_set_playback_switch(elem, SND_MIXER_SCHN_REAR_RIGHT, 1);
  }
	err = snd_mixer_selem_set_playback_volume(elem, 
		SND_MIXER_SCHN_FRONT_LEFT, left_vol);

#ifdef UNUSED_CODE
	if(err < 0)
	 fprintf(stderr,"snd_mixer_selem_set_playback_volume(FRONT LEFT):%s\n",
		snd_strerror(err));
#endif
	snd_mixer_selem_set_playback_volume(elem, 
		SND_MIXER_SCHN_REAR_LEFT, left_vol);
	snd_mixer_selem_set_playback_volume(elem, 
		SND_MIXER_SCHN_FRONT_RIGHT, right_vol);
	snd_mixer_selem_set_playback_volume(elem, 
		SND_MIXER_SCHN_REAR_RIGHT, right_vol);
   }
	snd_mixer_close(mixer_handle);

}/* Mixer_change_volume() */
