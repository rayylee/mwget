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
#include <cstring>
#include <cassert>

#include "url.h"
#include "macro.h"
#include "utils.h"

using namespace std;

/* the relative RFC documents are RFC1738, RFC1808, RFC2396, RFC2732 */

/* the pre_encode() and encode() functions don't always give the right
 * encoded url, when the url contians some request string like:
 * http://www.myget.org/example?work=yes no
 * it give a wrong encoded string:
 * http://www.myget.org/example?work=yes%20no
 * the correct string is:
 * http://www.myeget.org/example?work=yes+no */

#define XDIGIT_TO_XCHAR(x) (((x) < 10) ? ((x) + '0') : ((x) - 10 + 'A'))

#define XCHAR_TO_XDIGIT(x) \
	(((x) >= '0' && (x) <= '9') ? \
	((x) - '0') : (toupper(x) - 'A' + 10))

struct proto_struct
{
	char *protocolStr;
	Protocol protocol;
	int defaultPort;
	bool supported;
};

const static proto_struct protocol_table[] = {
	{"http://", HTTP, 80, true},
	{"ftp://", FTP, 21, true},
	{"sftp://", SFTP, 115, false},
#ifdef HAVE_SSL
	{"https://", HTTPS, 443, true},
#else
	{"https://", HTTPS, 443, false},
#endif
	{"mms://", MMS, 1755, false},
	{"rtsp://", RTSP, 554, false},
	{NULL, NONE, 0, false}
};

enum {
  urlchr_reserved = 1, // rfc1738 reserved chars
  urlchr_unsafe   = 2  // rfc1738 unsafe chars
};

/* Shorthands for the table: */
#define R  1 // reserved char
#define U  2 // unsafe char
#define RU 3 // R|U

const static unsigned char urlchr_table[256] =
{
  U,  U,  U,  U,   U,  U,  U,  U,   /* NUL SOH STX ETX  EOT ENQ ACK BEL */
  U,  U,  U,  U,   U,  U,  U,  U,   /* BS  HT  LF  VT   FF  CR  SO  SI  */
  U,  U,  U,  U,   U,  U,  U,  U,   /* DLE DC1 DC2 DC3  DC4 NAK SYN ETB */
  U,  U,  U,  U,   U,  U,  U,  U,   /* CAN EM  SUB ESC  FS  GS  RS  US  */
  U,  0,  U, RU,   R,  U,  R,  0,   /* SP  !   "   #    $   %   &   '   */
  0,  0,  0,  R,   R,  0,  0,  R,   /* (   )   *   +    ,   -   .   /   */
  0,  0,  0,  0,   0,  0,  0,  0,   /* 0   1   2   3    4   5   6   7   */
  0,  0, RU,  R,   U,  R,  U,  R,   /* 8   9   :   ;    <   =   >   ?   */
 RU,  0,  0,  0,   0,  0,  0,  0,   /* @   A   B   C    D   E   F   G   */
  0,  0,  0,  0,   0,  0,  0,  0,   /* H   I   J   K    L   M   N   O   */
  0,  0,  0,  0,   0,  0,  0,  0,   /* P   Q   R   S    T   U   V   W   */
  0,  0,  0, RU,   U, RU,  U,  0,   /* X   Y   Z   [    \   ]   ^   _   */
  U,  0,  0,  0,   0,  0,  0,  0,   /* `   a   b   c    d   e   f   g   */
  0,  0,  0,  0,   0,  0,  0,  0,   /* h   i   j   k    l   m   n   o   */
  0,  0,  0,  0,   0,  0,  0,  0,   /* p   q   r   s    t   u   v   w   */
  0,  0,  0,  U,   U,  U, RU,  U,   /* x   y   z   {    |   }   ~   DEL */

  U, U, U, U,  U, U, U, U,  U, U, U, U,  U, U, U, U,
  U, U, U, U,  U, U, U, U,  U, U, U, U,  U, U, U, U,
  U, U, U, U,  U, U, U, U,  U, U, U, U,  U, U, U, U,
  U, U, U, U,  U, U, U, U,  U, U, U, U,  U, U, U, U,

  U, U, U, U,  U, U, U, U,  U, U, U, U,  U, U, U, U,
  U, U, U, U,  U, U, U, U,  U, U, U, U,  U, U, U, U,
  U, U, U, U,  U, U, U, U,  U, U, U, U,  U, U, U, U,
  U, U, U, U,  U, U, U, U,  U, U, U, U,  U, U, U, U,
};

#define urlchr_test(c, mask) (urlchr_table[(unsigned char)(c)] & (mask))
#define URL_RESERVED_CHAR(c) urlchr_test(c, urlchr_reserved)
#define URL_UNSAFE_CHAR(c) urlchr_test(c, urlchr_unsafe)

URL::URL()
{
	url = NULL;
	path = NULL;
	host = NULL;
	user = NULL;
	password = NULL;
	dir = NULL;
	file = NULL;
	protocol = NONE;
};

URL::~URL()
{
	free_all();
};

URL::URL(URL& url)
{
	this->set_url(url.get_url());
};

URL& 
URL::operator = (URL& url)
{
	if(&url != this){
		this->set_url(url.get_url());
	}
	return *this;
};

/* some url not encoded , may be contain some unsafe charactors
 * so encodeed it and escape the unsafe charactors */
const char *
URL::pre_encode(const char *pre_url)
{
	char *str_new;
	int str_new_len;
	char *ptr, *pptr;

	assert(pre_url != NULL);
	/* get the length of the new string */
	ptr = (char*)pre_url;
	str_new_len = 0;
	while(*ptr != '\0'){
		if(*ptr == '%'){
			if(isxdigit(ptr[1]) && isxdigit(ptr[2])){ 
				str_new_len += 3;
				ptr += 3;
			}else{
				str_new_len += 3;
				ptr += 1;
			}
		}else if(URL_UNSAFE_CHAR(*ptr) && !URL_RESERVED_CHAR(*ptr)){
			str_new_len += 3;
			ptr += 1;
		}else{
			str_new_len += 1;
			ptr += 1;
		}
	}// end of while

	/* encode the url */
	str_new = new char[str_new_len + 1];
	ptr = (char*)pre_url;
	pptr = str_new;
	while(*ptr != '\0'){
		if(*ptr == '%'){
			if(isxdigit(ptr[1]) && isxdigit(ptr[2])){
				strncpy(pptr, ptr, 3);
				ptr += 3;
				pptr += 3;
			}else{
				strncpy(pptr, "%25", 3);
				ptr += 1;
				pptr += 3;
			}
		}else if(URL_UNSAFE_CHAR(*ptr) && !URL_RESERVED_CHAR(*ptr)){
			pptr[0] = '%';
			pptr[1] = XDIGIT_TO_XCHAR(((unsigned char)*ptr) >> 4);
			pptr[2] = XDIGIT_TO_XCHAR(((unsigned char)*ptr) & 0x0f);
			ptr += 1;
			pptr += 3;
		}else{
			*pptr = *ptr;
			ptr += 1;
			pptr += 1;
		}
	} // end of while

	*pptr = '\0';

	return str_new;
}; // end of pre_encode

/* encode the url, the returned value must be freed */
const char*
URL::encode(const char* url)
{
	char *url_enc;
	int url_enc_len;
	char *ptr, *pptr;

	for(url_enc_len=0, ptr=(char*)url; *ptr != '\0'; ptr ++){
		if(URL_UNSAFE_CHAR(*ptr) && !URL_RESERVED_CHAR(*ptr)){
			url_enc_len += 3;
		}else{
			url_enc_len += 1;
		}
	}

	url_enc = new char[url_enc_len + 1];
	for(pptr=url_enc, ptr=(char*)url; *ptr != '\0'; ptr ++){
		if(URL_UNSAFE_CHAR(*ptr) && !URL_RESERVED_CHAR(*ptr)){
			pptr[0] = '%';
			pptr[1] = XDIGIT_TO_XCHAR(((unsigned char)*ptr) >> 4);
			pptr[2] = XDIGIT_TO_XCHAR(((unsigned char)*ptr) & 0x0f);
			pptr += 3;
		}else{
			*pptr = *ptr;
			pptr += 1;
		}
	}

	*pptr = '\0';

	return url_enc;
}; // end of URL::encode

/* decode the encoded url, the returned value must be freed */
const char*
URL::decode(const char* url)
{
	char *url_dec;
	int url_dec_len;
	char *ptr, *pptr;

	for(url_dec_len=0, ptr=(char*)url; *ptr != '\0'; ptr++){
		if(*ptr == '%'){
			if(isxdigit(ptr[1]) && isxdigit(ptr[2])){
				url_dec_len += 1;
				ptr += 2;
			}else{
				return NULL;
			}
		}else{
			url_dec_len += 1;
		}
	}

	url_dec = new char[url_dec_len + 1];
	for(pptr=url_dec, ptr=(char*)url; *ptr != '\0'; ptr++){
		if(*ptr == '%'){
			pptr[0] = (XCHAR_TO_XDIGIT(ptr[1]) << 4) + (XCHAR_TO_XDIGIT(ptr[2])); 
			pptr ++;
			ptr += 2;
		/* if the filename contian '+' but not the request this is wrong 
		}else if(*ptr == '+'){
			*pptr = ' ';
			pptr ++;
		*/
		}else{
			*pptr = *ptr;
			pptr ++;
		}
	}
	
	*pptr = '\0';

	return url_dec;
}; // end of URL::decode

/* remove the fragment from the url */
int
URL::_parse_fragment(char* &url)
{
	char *ptr;

	for(ptr = url; *ptr != '\0'; ptr ++){
		if(*ptr == '#'){
			*ptr = '\0';
			return 0;
		}
	}

	return -1;
};

int
URL::_parse_scheme(char* &url)
{
	char *ptr;
	const proto_struct *ps;

	for(ps = protocol_table; ps->protocolStr != NULL; ps ++){
		if(strncmp(url, ps->protocolStr, strlen(ps->protocolStr)) == 0){
			protocol = ps->protocol;
			port = ps->defaultPort;
			url += strlen(ps->protocolStr);

			if(! ps->supported){
				return E_PROTO_NOT_SUPPORT;
			}else{
				return 0;
			}
		}
	}

	return -1;
};

int
URL::_parse_location(char* &url)
{
	char *ptr, *begin, *split;

	begin = url;
	
	for(ptr = begin; *ptr != '\0' && *ptr != '/'; ptr ++){
		if(*ptr == '@'){ // contain user info
			*ptr = '\0';
			for(split = begin; *split != '\0' && *split != ':'; split ++) ;
			if(*split == ':'){
				*split = '\0';
				user = decode(begin);
				password = decode(split + 1);
			}else{
				user = decode(begin);
			}
			begin = ptr + 1;
			break;
		}
	}

	ptr = begin;

	if(*begin == '['){ // ipv6 address
		begin ++;
		for(ptr = begin; *ptr != '\0' && *ptr != '/' && *ptr != ']'; ptr ++) ;
		if(*ptr != ']') return  E_BAD_URL;
		*ptr = '\0';
		ptr ++;
	}

	for(; *ptr != '\0'; ptr ++){
		if(*ptr == ':'){
			*ptr = '\0';
			port = atoi(ptr + 1);
		}else if(*ptr == '/'){
			*ptr = '\0';
			host = StrDup(begin);
			ptr ++;
			break;
		}
	}

	if(*ptr == '\0') host = StrDup(begin);

	url = ptr;

	return 0;
}; // end of _parse_location

int
URL::_parse_query_info(char* &url)
{
	char *ptr;

	for(ptr = url; *ptr != '\0' && *ptr != '?'; ptr ++) ;

	*ptr = '\0';
	return 0;
};

int
URL::_parse_parameters(char* &url)
{
	char *ptr;

	// remove ";type=a" from the url, the parameters maybe useful
	// for ftp protocol; we ignore this
	for(ptr = url; *ptr != '\0' && *ptr != ';'; ptr ++) ;

	*ptr = '\0';
	return 0;
};

/* According to the Ftp protocol
 * PATH			: ftp-command
 * %2Fetc/motd	: CWD /etc; RETR motd
 * etc/motd		: CWD etc; RETR motd
 * /etc/motd	: CWD; CWD etc; RETR motd 
 * there will be some differents when the user's home dir
 * is not the root dir how to do with this condition? */
int
URL::_parse_path(char* &url)
{
	char *ptr;

	ptr = strrchr(url, '/');

	if(ptr != NULL){
		if(*(ptr + 1) == '\0'){
			*ptr = '\0';
			dir = decode(url);
		}else{
			*ptr = '\0';
			dir = decode(url);
			if(*(ptr + 1) != '\0'){
				file = decode(ptr + 1);
			}
		}
	}else if(*url != '\0'){
		file = decode(url);
	}

	return 0;
};

int 
URL::_parse(const char *url_orig)
{
	char *url_parsed;
	char buf[1024];

	url = pre_encode(url_orig);
	snprintf(buf, 1024, "%s", url);
	url_parsed = buf;

	RETURN_IF_FAILED(_parse_scheme(url_parsed));

	RETURN_IF_FAILED(_parse_location(url_parsed));

	// the ftp-URL use &#num as the normal charactors,
	// so must special
	if(protocol == FTP){
		_parse_path(url_parsed);
		return 0;
	}

	_parse_fragment(url_parsed);

	// download;type=a?version=0.1
	path = new char[strlen(url_parsed) + 2];
	sprintf((char*)path, "/%s", url_parsed);
	_parse_query_info(url_parsed);

	// download;type=a
	RETURN_IF_FAILED(_parse_parameters(url_parsed));

	// download
	_parse_path(url_parsed);

	return 0;
};

int
URL::set_url(const char *url_orig)
{
	free_all();
	return _parse(url_orig);
};

void
URL::free_all(void)
{
	delete[] url; url = NULL;
	delete[] user; user = NULL;
	delete[] password; password = NULL;
	delete[] host; host = NULL;
	delete[] dir; dir = NULL;
	delete[] file; file = NULL;
	delete[] path; path = NULL;
	protocol = NONE;
};

int
URL::reset_url(const char *url_orig)
{
	assert(url_orig);

	char buf[1024];
	char *pro_str = NULL;
	const char *url_new;
	const proto_struct *ps;

	if(strstr(url_orig, "://")){
		url_new = url_orig;
	}else{
		for(ps = protocol_table; ps->protocolStr != NULL; ps ++){
			if(ps->protocol == protocol){
				pro_str = ps->protocolStr;
				break;
			}
		}
		if(pro_str == NULL) return -1;

		if(url_orig[0] == '/'){
			snprintf(buf, 1024,
					"%s%s%s%s%s%s:%d%s",
					pro_str,
					user ? user : "",
					password ? ":" : "",
					password ? password : "",
					user ? "@" : "",
					host,
					port,
					url_orig);
		}else{
			snprintf(buf, 1024,
					"%s%s%s%s%s%s:%d%s%s/%s",
					pro_str,
					user ? user : "",
					password ? ":" : "",
					password ? password : "",
					user ? "@" : "",
					host,
					port,
					dir ? "/" : "",
					dir ? dir : "",
					url_orig);
		}

		url_new = buf;
	}

	free_all();
	return _parse(url_new);
};

#undef R
#undef U
#undef RU
