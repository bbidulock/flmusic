#ifndef _COMMON_DEFS_H_
#define _COMMON_DEFS_H_

#include <linux/cdrom.h>

#define NR_CDFRAMES 12
#define BUF_MAXLEN CD_FRAMESIZE_RAW*NR_CDFRAMES

#define PLAYINFO_FILE "/tmp/.playinfo.txt"

/* PROTOTYPES */

void Player_set_progress(double v);

#endif /* _COMMON_DEFS_H_ */
