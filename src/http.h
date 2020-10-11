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

#ifndef _HTTP_H
#define _HTTP_H

#include <sys/types.h>
#include <unistd.h>

#include "advio.h"
#include "tcp.h"
#include "header.h"
#include "macro.h"
		
class Http
	: public PluginIO
{
	public:
		Http();
		~Http(){ delete conn; };

		// set timeout
		void set_timeout(long timeout);
		// set log function
		void set_log(void(*log)(const char *, ...));
#ifdef HAVE_SSL
		// set ues ssl
		void set_use_ssl(bool use);
#endif

		// connect to the http server
		int connect(const char *host, int port);
		int connect(const TcpSockAddr &addr);
		int connect();

		// add or mofify header info
		int header(const char *attrName, const char *attrValue);
		// set authorization info
		int auth(const char *user, const char *password);
		// set proxy authorization info
		int proxy_auth(const char *user, const char *password);
		// set range
		int set_range(off_t start, off_t end = -1);
		// set host
		int set_host(const char *host, int port);

		// send the head info
		int send_head();
		// Request use HEAD method
		int head(const char *url);
		// Request use GET method
		int get(const char *url);
		// Request use POST method
		int post(const char *url);

		// get and parse header from the connection
		int parse_header();
		// get the user defined header content
		const char* get_header(const char *attr_name);
		// get the status code returned from the server
		int get_status_code();
		// get filesize
		off_t get_file_size();
		// accept range or not
		bool accept_ranges();
		// read maxsize data from the data section
		int read_data(char *buffer, int maxsize);
		// is data_ends ?, this is not very resonable
		int data_ends();

	private:
		void (*log)(const char *, ...);
		static void default_log(const char *data, ...){};

	protected:
		TcpConnection *conn;
		HeadData request;
		HeadData response;
		TcpSockAddr remote;
		int statusCode;
		long timeout;
		char buf[1024];
		off_t fileSize;
		off_t chunkedSize;
		bool isChunked;
#ifdef HAVE_SSL
		bool useSSL;
#endif
};

#endif // _HEADER_H
