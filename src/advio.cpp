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


#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <cstring>

#include "advio.h"
#include "macro.h"
#include "plugin.h"

/*****************************************************
 * class IOStream implement
 *****************************************************/

IOStream::IOStream(int infd)
	:fd(infd)
{
#ifdef HAVE_SSL
	ssl = NULL;
	sslCTX = NULL;
	useSSL = false;
#endif
};

IOStream::~IOStream()
{ 
#ifdef HAVE_SSL
	if(useSSL){
		SSL_shutdown(ssl);
		free(ssl);
		if(sslCTX) SSL_CTX_free(sslCTX);
	}
#endif
	::close(fd);
};

int
IOStream::set_fd(int infd)
{
	fd = infd;
#ifdef HAVE_SSL
	if(useSSL){
		assert(ssl != NULL);
		SSL_set_fd(ssl, infd);
		int ret;
		if((ret=SSL_connect(ssl)) <= 0){
			SSL_get_error(ssl, ret);
			return -1;
		}
	}
#endif

	return 0;
};

int
IOStream::get_fd()
{
	return fd;
};

int
IOStream::open(const char *file, int flag, mode_t mode)
{
	return ( fd = ::open(file, flag, mode) );
};

int
IOStream::close()
{
	return ::close(fd);
};

#ifdef HAVE_SSL
void
IOStream::set_use_ssl(bool use)
{
	if(use){
		if(!useSSL){
			useSSL = true;
			free(ssl);
			if(sslCTX != NULL) SSL_CTX_free(sslCTX);
			sslCTX = SSL_CTX_new(SSLv23_client_method());
			ssl = SSL_new(sslCTX);
			SSL_set_fd(ssl, fd);
		}
	}else{
		useSSL =false;
		SSL_shutdown(ssl);
		free(ssl); ssl = NULL;
		if(sslCTX) SSL_CTX_free(sslCTX);
	}
};

int
IOStream::ssl_connect(void)
{
	assert(fd >= 0);
	if(!useSSL || ssl == NULL){
		return -1;
	}
	int ret;
	if((ret=SSL_connect(ssl)) <= 0){
		SSL_get_error(ssl, ret);
		return -1;
	}else{
		return 0;
	}
};
#endif // HAVE_SSL

int
IOStream::read(char *buffer, int maxsize, long timeout)
{
	fd_set r_fdset;
	int rc;
	struct timeval *time_out;
	struct timeval tmp;
	if(timeout < 0){ /* block */
		time_out = NULL;
	}else{
		tmp.tv_sec = timeout;
		tmp.tv_usec = 0;
		time_out = &tmp;
	}

	while(1){
		FD_ZERO(&r_fdset);
		FD_SET(fd, &r_fdset);
		switch(select(fd+1, &r_fdset, NULL, NULL, time_out)){
			case 0:
				/*timeout*/
				return E_TIMEOUT;
			case -1:
				/*error*/
				if(errno == EINTR) continue;
				return -1;
			default:
				/*some fd of 0-fd are activated*/
				if(FD_ISSET(fd, &r_fdset)){
					while(1){
#ifdef HAVE_SSL
						if(useSSL && ssl != NULL){
							rc = SSL_read(ssl, buffer, maxsize);
							if(rc > 0){
								return rc;
							}else if(rc == 0){ // error or EOF
								if(SSL_get_error(ssl, rc) == SSL_ERROR_ZERO_RETURN){
									return 0; // EOF
								}else{
									return -1;
								}
							}else{ // error
								return rc;
							}
						}
#endif
						if((rc=::read(fd, buffer, maxsize)) < 0){
							if(errno == EINTR)
								continue;
							return -1;
						}
						return rc; /*contain EOF and normal return*/
					}
				}
		}
	}
};
	
int
IOStream::write(char *buffer, int maxsize, long timeout)
{
	fd_set w_fdset;
	int rc;
	struct timeval *time_out;
	struct timeval tmp;
	if(timeout < 0){ /* block */
		time_out = NULL;
	}else{
		tmp.tv_sec = timeout;
		tmp.tv_usec = 0;
		time_out = &tmp;
	}


	while(1){
		FD_ZERO(&w_fdset);
		FD_SET(fd, &w_fdset);
		switch(select(fd+1, NULL, &w_fdset, NULL, time_out)){
			case 0:
				/*timeout*/
				return E_TIMEOUT;
			case -1:
				/*error*/
				if(errno == EINTR) continue;
				return -1;
			default:
				/*some fd of 0-fd are activated*/
				if(FD_ISSET(fd, &w_fdset)){
					while(1){
#ifdef HAVE_SSL
						if(useSSL && ssl != NULL){
							if((rc=SSL_write(ssl, buffer, maxsize)) <= 0){
								return -1;
							}else{
								return rc;
							}
						}
#endif
						if((rc=::write(fd, buffer, maxsize)) < 0){
							if(errno == EINTR)
								continue;
							return -1;
						}
						return rc;
					}
				}
		}
	}

};			

/*****************************************************************
 * class BufferStream implement
 ****************************************************************/
BufferStream::BufferStream(int ifd)
	: IOStream(ifd)
{
	ptr = buf;
	count = 0;
};

BufferStream::BufferStream(const BufferStream& that)
	: IOStream(-1)
{
	ptr = buf;
	count = 0;
};

BufferStream& 
BufferStream::operator = (const BufferStream& that)
{
	if(this == &that) return *this;

	ptr = buf;
	count = 0;
	fd = -1;
	return *this;
};

int
BufferStream::set_fd(int infd)
{
	buf[0] = '\0';
	ptr = buf;
	count = 0;
	return IOStream::set_fd(infd);
};

int
BufferStream::write(char *str, long timeout)
{
	int n, rc, slen;
	char *pptr;

	n = strlen(str);
	slen = n;
	pptr = str;

	while(n > 0){
		if((rc=IOStream::write(str, n, timeout)) > 0){
			n -= rc;
			pptr += rc;
		}else
			return rc;
	}

	return slen;
};

int
BufferStream::read(char *buffer, int maxsize, long timeout)
{
	if(count > 0){
		int min;
		min = count < maxsize ? count : maxsize;
		memcpy(buffer, ptr, min);
		count -= min;
		ptr += min;
		return min;
	}else{
		return IOStream::read(buffer, maxsize, timeout);
	}
};

int
BufferStream::readc(char *c, long timeout)
{
	if(count <= 0){
		if((count=IOStream::read(buf, BUFSIZE, timeout)) < 0)
			return count; //error
		else if(count == 0)
			return 0; //EOF
		ptr = buf;
	}

	*c = *ptr;
	ptr ++;
	count --;

	return 1;
};

int
BufferStream::read_line(char *line, int maxsize, long timeout)
{
	int i, rc;
	char c;
	char *pptr;

	pptr = line;

	for(i=1; i<maxsize; i++){
		if((rc=readc(&c, timeout)) < 0){
			return rc; //error
		}else if(rc == 0){ //EOF
			if(i == 1){//read nothing but EOF
				return 0;
			}else{
				break;
			}
		}
		*pptr = c;
		pptr ++;
		if(c == '\n')
			break;
	}

	*pptr = '\0';

	if(i == maxsize)
		return i-1;
	else
		return i;
};

/****************************************************
 * class PluginIO implement
 ****************************************************/
PluginIO::PluginIO()
{
	// do nothing
};

PluginIO::~PluginIO()
{
	// do nothing
};

int
PluginIO::read_data(char *buffer, int maxsize)
{
	// this function is a virtual function and must
	// be overloaded by the base class of the plugin
	return -1;
};

/****************************************************
 * class BufferFile implement
 ****************************************************/

BufferFile::BufferFile()
{
	ptr = buf;
	left = FILE_BUFFER_SIZE;
	fd = -1;
};

BufferFile::~BufferFile()
{
	flush();
	::close(fd);
};

int
BufferFile::open(const char *file, int flag, mode_t mode)
{
	ptr = buf;
	left = FILE_BUFFER_SIZE;
	return ( fd = ::open(file, flag, mode) );
};

int
BufferFile::close()
{
	flush();
	return ::close(fd);
};

// flush the data to the hard-disk, and return the bytes
// on success, return -1 on error
int
BufferFile::flush()
{
	int count;
	int wc;
	char *pptr;
	int bc;

	count = FILE_BUFFER_SIZE - left;
	bc = count;
	pptr = buf;
	while(count > 0){
_flush_again:
		wc = ::write(fd, pptr, count);
		if(wc < 0){
			if(errno == EINTR) goto _flush_again;
			perror("write back to file failed");
			break;
		}
		pptr += wc;
		count -= wc;
	}

	ptr = buf;
	if(count > 0){ // some error happend
		memmove(buf, pptr, count);
		left += bc - count;
		return -1;
	}else{
		left = FILE_BUFFER_SIZE;
		return bc;
	}
};

off_t
BufferFile::seek(off_t off_set)
{
	// before seek we must flush the buffer,
	// if not we will write the data to the wrong postion
	flush();
	return lseek(fd, off_set, SEEK_SET);
};

int
BufferFile::truncate(off_t length)
{
	return ftruncate(fd, (off_t)length);
};

// get length bytes data from pio, and argument rtlength
// is used to real-time reflect the change, if the length is set
// to -1, this function will read all the data
off_t
BufferFile::retr_data_from(PluginIO *pio, off_t *rtlength, off_t length)
{
	assert(pio != NULL);
	int ret;
	off_t data_count = 0;

	while(1){
		ret = pio->read_data(ptr, left);
		if(ret == 0){
			break;
		}else if(ret < 0){
			return ret;
		}
		left -= ret;
		ptr += ret;
		*rtlength += ret;
		data_count += ret;
		if(left <= MIN_BUFFER_LEFT){
			flush();
		}
		if(length >= 0 && data_count >= length) break;
	}

	return data_count;
};
