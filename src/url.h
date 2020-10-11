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


#ifndef _URL_H
#define _URL_H
// define some protocol type
enum Protocol{ 
	HTTP,
	FTP,
	SFTP,
	HTTPS,
	MMS,
	RTSP,
	NONE
};

// parse the URL
class URL
{
	public:
		URL();
		URL(URL& url);
		URL& operator = (URL& url);
		// free the stack
		~URL();

		// set the url
		int set_url(const char *url);
		// reset the url
		int reset_url(const char *url);
		// the the encoded url
		const char* get_url(){ return url; };
		// get the protocol
		Protocol get_protocol(){ return protocol; };
		// get the user
		const char* get_user(){ return user; };
		// get the user's password
		const char* get_password(){ return password; };
		// get the host
		const char* get_host(){ return host; } ;
		// get the port
		int get_port(){ return port; };
		// get the directory
		const char* get_dir(){ return dir; };
		// get the file
		const char* get_file(){ return file; };
		// get encode url and file needed by http protocol
		const char* get_encoded_path(){ return path; };
		// decode(), encode() and pre_encode() return the allocate memory, so need free
		static const char* decode(const char* url);
		static const char* encode(const char* url);
		static const char* pre_encode(const char* pre_url);
	private:
	
		int _parse_fragment(char* &url);
		int _parse_scheme(char* &url);
		int _parse_location(char* &url);
		int _parse_query_info(char* &url);
		int _parse_parameters(char* &url);
		int _parse_path(char* &url);
		int _parse(const char *url);
		void free_all(void);

		const char *url; // the encoded url
		const char *path;
		Protocol protocol;
		const char *user;
		const char *password;
		const char *host;
		int port;
		const char *dir;
		const char *file;
};

#endif // _URL_H
