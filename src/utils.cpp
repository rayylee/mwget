/*  Myget - A download accelerator for GNU/Linux
 *  Homepage: http://myget.sf.net
 *  Copyright (C) 2005- xiaosuo
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <iostream>
#include <cstdio> //Patch of gcc-4.3.Wan Zhi Yuan
#include <termios.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <sys/time.h>
#include <cassert>
#include <cstring>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "utils.h"


using namespace std;

char*
StrDup(const char *str)
{
	char *ptr;
	int i;

	if(str == NULL) return NULL;

	for(i = 0; str[i] != '\0'; i ++) ;
	ptr = new char[i + 1];
	for(i = 0; str[i] != '\0'; i ++){
		ptr[i] = str[i];
	}
	ptr[i] = '\0';

	return ptr;
};

/* the relative RFC: RFC 3548 */
const char* 
base64_encode(const char *str, int length)
{  
	static const char base64_table[64] = {
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
    'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
    'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
    'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
    'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
    'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
    'w', 'x', 'y', 'z', '0', '1', '2', '3',
    '4', '5', '6', '7', '8', '9', '+', '/',
  };

	int str_enc_len;
	char *str_enc;
	unsigned char *ptr, *pptr;
	int i;
	int length_mod;

	length = (length < 0 ? strlen(str) : length);

	length_mod = length % 3;
	length /= 3;
	str_enc_len = length * 4;
	str_enc_len += ((length_mod == 0) ? 0 : 4);

	str_enc = new char[str_enc_len + 1];

	ptr = (unsigned char*)str;
	pptr = (unsigned char*)str_enc;
	for(i = 0; i < length; i ++){
		*pptr++ = base64_table[(ptr[0] >> 2)];
		*pptr++ = base64_table[((ptr[0] & 0x3) << 4) + (ptr[1] >> 4)];
		*pptr++ = base64_table[((ptr[1] & 0xf) << 2) + (ptr[2] >> 6)];
		*pptr++ = base64_table[(ptr[2] & 0x3f)];
		ptr += 3;
	}

	if(length_mod == 1){
		*pptr++ = base64_table[(ptr[0] >> 2)];
		*pptr++ = base64_table[((ptr[0] & 0x3) << 4)];
		*pptr++ = '=';
		*pptr++ = '=';
	}else if(length_mod == 2){
		*pptr++ = base64_table[(ptr[0] >> 2)];
		*pptr++ = base64_table[((ptr[0] & 0x3) << 4) + (ptr[1] >> 4)];
		*pptr++ = base64_table[((ptr[1] & 0xf) << 2)];
		*pptr++ = '=';
	}

	*pptr = '\0';

	return str_enc;
};

/* the decode function is not useful in this software so no need to write it
   static const char* 
   base64_decode(const char *str, int longth = -1)
   {
   };
   */

/* Determine the width of the terminal we're running on.  If that's
 * not possible, return 0.  */
int
determine_screen_width(void)
{
#ifndef TIOCGWINSZ
	return 0;
#else  /* TIOCGWINSZ */
	struct winsize wsz;

	if(ioctl(fileno(stderr), TIOCGWINSZ, &wsz) < 0) return 0;

	return wsz.ws_col;
#endif /* TIOCGWINSZ */
}

double
get_current_time(void)
{
	struct timeval time[1];

	if(gettimeofday(time, 0) < 0) return -1;
	
	return (double)time->tv_sec + (double)time->tv_usec / 1000000 ;
}

// conver size to 333M, 111K, 1G
void
convert_size(char *sizeStr, off_t size)
{
	double dsize = size;

	if(dsize < 0){
		sprintf(sizeStr, "%3ldB", 0);
		return;
	}

	if(dsize < 1000){
		sprintf(sizeStr, "%3ldB", (long)dsize);
		return;
	}
	dsize /= 1024;
	if(dsize < 1000){
		if(dsize <= 9.9){
			sprintf(sizeStr, "%.1fK", dsize);
		}else{
			sprintf(sizeStr, "%3ldK", (long)dsize);
		}
		return;
	}
	dsize /= 1024;
	if(dsize < 1000){
		if(dsize <= 9.9){
			sprintf(sizeStr, "%.1fM", dsize);
		}else{
			sprintf(sizeStr, "%3ldM", (long)dsize);
		}
		return;
	}
	dsize /= 1024;
	if(dsize < 1000){
		if(dsize <= 9.9){
			sprintf(sizeStr, "%.1fG", dsize);
		}else{
			sprintf(sizeStr, "%3ldG", (long)dsize);
		}
		return;
	}
};
// convert time to 11d23 11h12 12:34
void
convert_time(char *timeStr, double time)
{
	long sec, min, hour, day;
	
	min = (long)time / 60; // min
	sec = (long)time % 60; // sec

	if(min < 60){
		sprintf(timeStr, "%02d:%02d", min, sec);
		return;
	}

	hour = min / 60;
	min %= 60;

	if(hour < 24){
		sprintf(timeStr, "%2dh%2d", hour, min);
		return;
	}

	day = hour / 24;
	hour %= 24;

	if(day < 100){
		sprintf(timeStr, "%2dd%2d", day, hour);
		return;
	}
	
	sprintf(timeStr, "--:--");
};

bool
file_exist(const char *file)
{
	int fd;
	
	assert(file != NULL);

	fd = open(file, O_RDONLY);
	if(fd < 0){
		return false;
	}else{
		close(fd);
		return true;
	}
};
