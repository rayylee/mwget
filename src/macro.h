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

#ifndef _MACRO_H
#define _MACRO_H

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#ifdef HAVE_SSL
#	include <openssl/ssl.h>
#endif

// #define DEBUG

// ftp passive or active
#define PASV 0
#define PORT 1

// the returned value
#define S_REDIRECT 2
#define S_USER_TERM 1
#define SUCC 0
#define E_SYS -1
#define E_TIMEOUT -2
#define E_COMMON -3
#define E_RESOLVE -4
#define E_PROTO_NOT_SUPPORT -5
#define E_BAD_URL -6
#define E_BAD_HEADER -7
#define E_MAX_COUNT -8
#define E_OPEN_LOCALE_FILE -9
#define E_LIST_DIR -10
#define E_SSL_CONN -11

// the default sock read buffer size in linux
#define DEFAULT_SOCKET_READ_BUFFER_SIZE 102*1024
// APU 3.9 's result is 8129 bytes is the best buffer size
#define FILE_BUFFER_SIZE 1024*1024
#define MIN_BUFFER_LEFT 1024

#endif // _MACRO_H
