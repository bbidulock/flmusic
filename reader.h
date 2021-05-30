#ifndef _CD_READER_H_
#define _CD_READER_H_

#define MAX_TRACKS  80

typedef struct track_info
{
	char *artist;
	char *album;
	char *track;

} TrackInfo;

typedef struct cd_track_list
{
	int first_track, last_track;
	int *minutes;
	int *seconds;
	int *frames;
	int *track_start;
	char *types;
	int first_data_track, last_data_track, has_data_tracks;
}TrackList;

typedef struct play_info
{
	TrackInfo tracks[MAX_TRACKS];
	TrackList list;

	int cdrom_fd;
	int track_len;
	int track_start;
	int frame_pos;
    int nr_tracks;
    int start_track;
    int cur_track;

	const char *device_name;
}PlayInfo;

/* PROTOTYPES */

extern PlayInfo *Reader_open(const char *device_name);
extern void Reader_close(PlayInfo *rinfo);
extern int Reader_read_cd(int cdrom_fd, int lba, int num, unsigned char *buf);

extern void Reader_open_tray(const char *device_name);
extern void Reader_close_tray(const char *device_name);

extern int Reader_disk_ok(void);

#endif /* _CD_READER_H_ */
