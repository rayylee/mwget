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

/*some advance IO classes and functions*/

#ifndef _ADVIO_H
#define _ADVIO_H

#define MAXLONG 2147483647 // 2G
#define BUFSIZE 1024

#include <cassert>
#include <fcntl.h>

#include "macro.h"

/* class IOStream add timeout option support*/

class IOStream
{
	public:
		/* default fd == -1, it is safe */
		IOStream(int infd=-1);
		~IOStream();

		int set_fd(int infd);
		int get_fd();

		int open(const char *file, int flag=O_RDWR|O_CREAT, mode_t mode=00644);
		int close();

		/* when timeout == -1, it is blocked */
		int read(char *buffer, int maxsize, long timeout=-1);
		int write(char *buffer, int maxsize, long timeout=-1);

#ifdef HAVE_SSL
		void set_use_ssl(bool use);
		int ssl_connect(void);
#endif

	protected:
		int fd;
#ifdef HAVE_SSL
		SSL *ssl;
		SSL_CTX *sslCTX;
		bool useSSL;
#endif
};

/* extend timeout input & output stream*/
class BufferStream 
	: public IOStream
{
	public:
		BufferStream(int ifd=-1);
		//~BufferStream();
		BufferStream(const BufferStream& that);
		BufferStream& operator = (const BufferStream& that);

		int set_fd(int infd);

		// the below functions have buffer supported
		int readc(char *c, long timeout=-1);
		int read(char *buffer, int maxsize, long timeout=-1);
		int write(char *str, long timeout=-1);
		int read_line(char *line, int maxsize,long timeout=-1);

	protected:
		char buf[BUFSIZE];
		char *ptr;
		int count; // the current buffer size
};

// this class must be inherited by the base class of the 
// plugin and the read_data function must be overloaded
class PluginIO
{
	public:
		PluginIO();
		~PluginIO();

		virtual int read_data(char *buffer, int maxsize);
};

class BufferFile
{
	public:
		BufferFile();
		~BufferFile();

		int open(const char *file, int flag=O_RDWR|O_CREAT, mode_t mode=00644);
		int close();
		int read(char *buffer, int maxsize);
		int write(char *buffer, int maxsize);

		int flush();
		off_t seek(off_t off_set);
		int truncate(off_t length);

		off_t retr_data_from(PluginIO *pio, off_t *rtlength, off_t length = -1);

	public:
		char buf[FILE_BUFFER_SIZE];
		char *ptr;
		int left;
		int fd;
};

#endif // _ADVIO_H
