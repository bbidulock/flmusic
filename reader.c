#include <config.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>

#include <linux/cdrom.h>
#include <alsa/asoundlib.h>

#include <FL/Fl.H>
#include <FL/fl_ask.H>

#include "audio.h"
#include "reader.h"

static int disk_ok = -1;

int Reader_disk_ok(void)
{
    return disk_ok;
}

static int cd_status(int fd)
{
	int i;

	disk_ok = 0;

test_status:
	i = ioctl(fd, CDROM_DRIVE_STATUS, CDSL_CURRENT);

	switch(i)
   {
	case CDS_NO_INFO:
		fputs("got NO_INFO\n", stderr);
		return 0;

	case CDS_TRAY_OPEN:
		if(ioctl(fd, CDROMCLOSETRAY) != 0) 
	   {
		perror("CDS_TRAY_OPEN failed");
		return 0;
	   }
		goto test_status;

	case CDS_NO_DISC:
		perror("CDS_NO_DISC");
		return 0;

	case CDS_DRIVE_NOT_READY:
		if(errno == 0) break;
		perror("DRIVE_NOT_READY");
		return 0;

	case CDS_DISC_OK:
		break;

	default:
		fl_alert("Unknown CD-ROM status(%d)\n", i);
		break;
   }
	i = ioctl(fd, CDROM_DISC_STATUS, CDSL_CURRENT);

	if(i == CDS_AUDIO || i == CDS_MIXED) 
   {
	disk_ok = 1; return 1;
   }
	const char *cs;
	if(i == CDS_DATA_1) cs = "CDS_DATA_1";
	else
	if(i == CDS_DATA_2) cs = "CDS_DATA_2";
	else
	if(i == CDS_XA_2_1) cs = "CDS_XA_2_1";
	else
	if(i == CDS_XA_2_2) cs = "CDS_XA_2_2";
	else
	 cs = "Unknow CDROM_DISC_STATUS";

	fl_alert("No audio CD in tray\n%d means %s", i, cs);
	return 0;
}

int Reader_read_cd(int cdrom_fd, int lba, int nr_frames, unsigned char *buf)
{
	struct cdrom_read_audio ra;

	ra.addr.lba = lba;
	ra.addr_format = CDROM_LBA;
	ra.nframes = nr_frames;
	ra.buf = buf;
	if(ioctl(cdrom_fd, CDROMREADAUDIO, &ra)) 
   {
	fl_alert("Reader_read_cd: read raw ioctl failed at lba(%d) nr_frames"
	 "(%d)\n\nerror:%s",lba,nr_frames,strerror(errno) );
	return 0;
   }
	return 1;
}

static void toc_fail(TrackList *list)
{
	free(list->track_start);
	free(list->types);
	free(list->minutes);
	free(list->seconds);
	free(list->frames);
	list->track_start = NULL;
	list->types = NULL;
	list->minutes = NULL;
	list->seconds = NULL;
	list->frames = NULL;
}

static int allocate_list_info(TrackList *list)
{
	int n = list->last_track - list->first_track + 2;

	if((list->track_start=(int*)calloc(n, sizeof(int)))==NULL)
   {
	return 0;
   }
	if((list->types=(char*)calloc(1, n))==NULL)
   {
	return 0;
   }
	if((list->minutes=(int*)calloc(n, sizeof(int)))==NULL)
   {
	return 0;
   }
	if((list->seconds=(int*)calloc(n,sizeof(int)))==NULL)
   {
	return 0;
   }
	if((list->frames=(int*)calloc(n,sizeof(int)))==NULL)
   {
	return 0;
   }
	return 1;
}

void Reader_open_tray(const char *device_name)
{
	int err, fd;

	fd = open(device_name, O_RDONLY | O_NONBLOCK);

	if(fd < 0)
   {
	fl_alert("Can not open tray\n%s", strerror(errno) );
	return;
   }

    if((err = ioctl(fd, CDROMEJECT)) != 0) 
   {
	fl_alert("OPEN TRAY failed\n%s", strerror(errno));
   }
	close(fd);
}

void Reader_close_tray(const char *device_name)
{
	int err, fd;

	fd = open(device_name, O_RDONLY);

	if(fd < 0)
   {
	return;
   }
	close(fd);
}

static int get_cd_info(const char *device_name, TrackList *list, int *out_fd)
{
	int i, k, fd, first, last, last_audio;
	struct cdrom_tochdr header;
	struct cdrom_tocentry entry;

	fd = open(device_name, O_RDONLY | O_NONBLOCK); *out_fd = fd;

	if(fd < 0)
   {
	fl_alert("get_cd_info: error opening device %s\n",device_name);

	return 0;
   }
	if(cd_status(fd) == 0) return 0;

	if((i = ioctl(fd, CDROMREADTOCHDR, &header)))
   {
	fl_alert("get_cd_info: CDROMREADTOCHDR failed with\n'%s'\n",
		strerror(errno));

	return 0;
   }

	list->first_track = first = (int)header.cdth_trk0;
	list->last_track  = last  = (int)header.cdth_trk1;

	if( !allocate_list_info(list))
   {
	fl_alert("get_cd_info: list data allocation failed\n");
	toc_fail(list);
	return 0;
   }

    entry.cdte_track = CDROM_LEADOUT;
    entry.cdte_format = CDROM_LBA;

	if(ioctl(fd,CDROMREADTOCENTRY, &entry))
   {
	fl_alert("get_cd_info: read TOC entry ioctl failed\n");
	toc_fail(list);
	return 0;
   }
	list->track_start[last] = entry.cdte_addr.lba;

#ifdef INFO_ONLY
linux/cdrom.h :
/* bit to tell whether track is data or audio (cdrom_tocentry.cdte_ctrl) */
#define CDROM_DATA_TRACK    0x04
#endif
	list->types[last] = entry.cdte_ctrl&CDROM_DATA_TRACK;

	if(list->types[last]) last_audio = last - 1; else last_audio = last;

	entry.cdte_track = CDROM_LEADOUT;
    entry.cdte_format = CDROM_MSF;

	if(ioctl(fd,CDROMREADTOCENTRY,&entry))
   {
	fl_alert("get_cd_info: read TOC entry ioctl failed\n");
	toc_fail(list);
	return 0;
   }
	list->minutes[last] = entry.cdte_addr.msf.minute;
	list->seconds[last] = entry.cdte_addr.msf.second;
	list->frames[last] = entry.cdte_addr.msf.frame;
	k = first - 1;

	for(i = first + 1; i <= last; ++i)
   {
    entry.cdte_track = i; ++k;
    entry.cdte_format = CDROM_LBA;

	if(ioctl(fd,CDROMREADTOCENTRY, &entry))
  {
	fl_alert("get_cd_info: read TOC entry ioctl failed\n");
	toc_fail(list);
	return 0;
  }
	list->track_start[k] = entry.cdte_addr.lba;
	list->types[k] = entry.cdte_ctrl&CDROM_DATA_TRACK;

	if(list->types[k] && (k < last_audio)) last_audio = k;
    entry.cdte_format = CDROM_MSF;

	if(ioctl(fd,CDROMREADTOCENTRY,&entry))
  {
	fl_alert("get_cd_info: read TOC entry ioctl failed\n");
	toc_fail(list);
	return 0;
  }
	list->minutes[k] = entry.cdte_addr.msf.minute;
	list->seconds[k] = entry.cdte_addr.msf.second;
	list->frames[k] = entry.cdte_addr.msf.frame;

   }
/* [szukw000] I found the following code in: 
 * cdparanoia-III-10.2/interface/common_interface.c
*/
	if(last_audio < last)/* CDS_MIXED */
   {
	struct cdrom_multisession multi;

	multi.addr_format = CDROM_LBA;

	if(ioctl(fd, CDROMMULTISESSION, &multi) != -1)
  {
	if(multi.addr.lba > 100) 
 {
	int start = multi.addr.lba - 11400;

	if(list->track_start[last_audio] > start
	&& start > list->track_start[last_audio - 1])
	 list->track_start[last_audio] = start;
 }
  }
   }
	list->last_track = last_audio;
	if(last_audio < last)
   {
	list->has_data_tracks = 1;
	list->first_data_track = last_audio + 1;
	list->last_data_track = last;
   }
	return 1;

}/* get_cd_info() */

PlayInfo *Reader_open(const char *device_name)
{
	PlayInfo *play;

	if((play = (PlayInfo*)calloc(1, sizeof(PlayInfo))) == NULL)
   {
	return NULL;
   }		
	play->cdrom_fd = -1;

	if( !get_cd_info(device_name, &play->list, &play->cdrom_fd)) 
   {
	free(play); return NULL;
   }
	play->cur_track = play->list.first_track;
	play->device_name = device_name;

	play->track_start = play->list.track_start[play->cur_track-1];
	play->track_len = play->list.track_start[play->cur_track] - 
		play->list.track_start[play->cur_track-1];

	return play;
}/* Reader_open() */

void Reader_close(PlayInfo *play)
{
	int i = 0;

	if(!play) return;

	if(play->cdrom_fd >= 0) 
   {
	close(play->cdrom_fd);
   }
	for (i = 0; i < MAX_TRACKS;i++) 
	{
	if(play->tracks[i].album) 
  {
	free(play->tracks[i].album);
  }	
	if(play->tracks[i].artist) 
  {
	free(play->tracks[i].artist);
  }	
	if(play->tracks[i].track) 
  {
	free(play->tracks[i].track);
  }	
	}	

	if(play->list.track_start) free(play->list.track_start);
	play->list.track_start = NULL;

	if(play->list.types) free(play->list.types);
	play->list.types = NULL;

	if(play->list.minutes) free(play->list.minutes);
	play->list.minutes = NULL;

	if(play->list.seconds) free(play->list.seconds);
	play->list.seconds = NULL;

	if(play->list.frames) free(play->list.frames);
	play->list.frames = NULL;

	free(play);
}/* Reader_close() */
