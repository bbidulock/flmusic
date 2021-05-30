#include <config.h>
#ifdef HAVE_LIBSOX
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <endian.h>

#if BYTE_ORDER == LITTLE_ENDIAN
#define MACHINE_IS_BIGENDIAN 0
#else
#define MACHINE_IS_BIGENDIAN 1
#endif

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#include <sox.h>
           }
#else
#include <sox.h>
#endif /* __cplusplus */

#include "file_info.h"

/* sox/src/util.c: */
#if SOX_LIB_VERSION_CODE >= SOX_LIB_VERSION(14,3,0)
extern char const * lsx_sigfigs3(double number);
#else
extern char const * lsx_sigfigs3(size_t number);
#endif

static const char *str_time(double seconds)
{
	static char buf[32];

	int hours, mins = seconds / 60;
	seconds -= mins * 60;
	hours = mins / 60;
	mins -= hours * 60;

	snprintf(buf, 31, "%02i:%02i:%05.2f", hours, mins, seconds);

	return buf;
}

static char const *size_and_bitrate(sox_format_t *ft, char const **text)
{
	struct stat st;    /* ft->fp may validly be NULL, so stat not fstat */

	if(stat(ft->filename, &st) 
	|| (st.st_mode & S_IFMT) != S_IFREG)
	 return NULL;

	if(ft->signal.length 
	&& ft->signal.channels 
	&& ft->signal.rate 
	&& text) 
   {
	double secs = 
	 ft->signal.length / ft->signal.channels / ft->signal.rate;
#if SOX_LIB_VERSION_CODE >= SOX_LIB_VERSION(14,3,0)
    *text = lsx_sigfigs3(8. * (double)st.st_size / secs);
#else
    *text = lsx_sigfigs3((size_t)(8. * (double)st.st_size / secs));
#endif
   }
#if SOX_LIB_VERSION_CODE >= SOX_LIB_VERSION(14,3,0)
    return lsx_sigfigs3((double)st.st_size);
#else
    return lsx_sigfigs3((size_t)st.st_size);
#endif
}

void FileInfo_write(sox_format_t *ft, FILE *writer)
{
	char const *text, *text2 = NULL;
	size_t ws;
	char buffer[30];

	ws = ft->signal.length / ft->signal.channels;


	fprintf(writer, "\n");

	if(ft->filename[0]) 
   {
	char *f = strdup(ft->filename);
	char *s = strrchr(f, '/');

	if(s) *s = 0; else s = f - 1;
	fprintf(writer, " Input File: %s", s+1); free(f);

	if(ft->handler.flags & SOX_FILE_DEVICE)
	 fprintf(writer, ": (%s)", ft->handler.names[0]);
	fprintf(writer, "\n");
   }

	if((text = size_and_bitrate(ft, &text2))) 
   {
	fprintf(writer, "  File Size: %-10s\n", text);
	if(text2)
	 fprintf(writer,"   Bit Rate: %s\n", text2);
   }
	fprintf(writer, "   Encoding: %-14s\n", 
	 sox_encodings_info[ft->encoding.encoding].name);

	sprintf(buffer, "   Channels: %u @ %u-bit", 
	 ft->signal.channels, ft->signal.precision);
	fprintf(writer, "%-25s\n", buffer);

	text = 
	 sox_find_comment(ft->oob.comments, "Tracknumber");
	if(text) 
   {
	fprintf(writer, "      Track: %s", text);
	text = sox_find_comment(ft->oob.comments, "Tracktotal");

	if(text)
	 fprintf(writer, " of %s", text);
	fprintf(writer, "\n");
   }

	sprintf(buffer, "Sample Rate: %gHz", ft->signal.rate);
	fprintf(writer, "%-25s\n", buffer);
	fprintf(writer, "   Duration: %-13s\n", 
	 ft->signal.length? str_time((double)ws / ft->signal.rate) : "unknown");

    if( !(ft->handler.flags & SOX_FILE_DEVICE) && ft->oob.comments)
   {
    if(sox_num_comments(ft->oob.comments) > 1)
  {
    sox_comments_t p = ft->oob.comments;
    fprintf(writer, "   Comments:\n");
    do
     fprintf(writer, "       %s\n", *p);
    while(*++p);
  }
    else
    fprintf(writer, "    Comment: '%s'\n", ft->oob.comments[0]);
   }
	else
   {

	if((text = sox_find_comment(ft->oob.comments, "Album")))
	 fprintf(writer, "      Album: %s\n", text);

	if((text = sox_find_comment(ft->oob.comments, "Artist")))
	 fprintf(writer, "     Artist: %s\n", text);

	if((text = sox_find_comment(ft->oob.comments, "Title")))
	 fprintf(writer, "      Title: %s\n", text);

	if((text = sox_find_comment(ft->oob.comments, "Year")))
	 fprintf(writer, "       Year: %s\n", text);
   }
}

#endif /* HAVE_LIBSOX */
