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


#include <cassert>
#include <sys/types.h>
#include <unistd.h>
#include <cstring>
#include <stdlib.h>
#include <stdio.h>

#include "ftp.h"
#include "tcp.h"
#include "advio.h"
#include "utils.h"
#include "debug.h"

/********************************************
 * Ftp implement
 * ******************************************/

Ftp::Ftp()
{
	ctrlConn = NULL;
	dataConn = NULL;
	timeout = -1;
	mode = PASV;
	stateLine[0] = 0;
	log = &default_log;
};

Ftp::~Ftp()
{
	delete ctrlConn;
	delete dataConn;
};

int
Ftp::connect(const char *addrstr, int port)
{
	assert(addrstr);

	Address addr;
	int ret;

	log(_("Resolve address...\n"));
	if(addr.resolve(addrstr, port) < 0) return E_RESOLVE;
	log(_("Connecting...\n"));
	ctrlConn = TcpConnector::connect(addr, ret, timeout);
	if(!ctrlConn) return ret;

	ctrlConn->get_remote_addr(remoteAddr);
	ctrlConn->get_local_addr(localAddr);
	return 0;
};

int
Ftp::connect(const TcpSockAddr& sock)
{
	int ret;

	log(_("Connecting...\n"));
	ctrlConn = TcpConnector::connect(sock, ret, timeout);
	if(!ctrlConn) return ret;

	ctrlConn->get_remote_addr(remoteAddr);
	ctrlConn->get_local_addr(localAddr);
	return 0;
};

int
Ftp::reconnect()
{
	int ret;
	log(_("Reconnecting...\n"));
	delete ctrlConn; ctrlConn = NULL;
	delete dataConn; dataConn = NULL;
	ctrlConn = TcpConnector::connect(remoteAddr, ret, timeout);
	if(!ctrlConn) return ret;

	return 0;
};

int
Ftp::ftp_cmd(const char* cmd, const char* args)
{
	int ret;
	char buffer[1024];
	int len;

	if(cmd){
		snprintf(buffer, 1024, "%s%s%s\r\n", cmd, 
				args == NULL ? "" : " " ,
				args == NULL ? "" : args);
		log("%s", buffer);
		if((ret=ctrlConn->write(buffer, timeout)) < 0)
			return ret;
	}

	do{
		ret = ctrlConn->read_line(buffer, 1024, timeout);
		if(ret < 0){
			return ret;
		}else if(ret == 0){
			return -1;
		}
		log("%s", buffer);
	}while(buffer[3] != ' ' || !ISDIGIT(buffer[0]));

	strcpy(stateLine, buffer + 4);

	return atoi(buffer);
};

int
Ftp::login(const char* user, const char* password)
{
	int ret;

	// get all the welcome message
	ret = ftp_cmd();
	switch(ret){
		case 220:
			break;
		default:
			return ret;
	}

	ret = ftp_cmd("USER", user == NULL ? "anonymous" : user);
	switch(ret){
		case 230:
			return 0;
		case 331:
			break;
		default:
			return ret;
	}

	ret = ftp_cmd("PASS", password == NULL ? "myget@myget" : password);
	switch(ret){
		case 230:
			return 0;
		default:
			return ret;
	}
};

int
Ftp::cwd(const char* dir)
{
	int ret;

	ret = ftp_cmd("CWD", dir ? dir : "/");
	switch(ret){
		case 250:
			return 0;
		default:
			return ret;
	}
};

int
Ftp::pwd(char** dir)
{
	int ret;
	char *ptr;
	char *tdir;
	
	ret = ftp_cmd("PWD");
	switch(ret){
		case 257:
			break;
		default:
			return ret;
	}

	for(ptr = stateLine; *ptr != '\0' && *ptr != '"'; ptr ++) ;
	if(*ptr != '"') return -1;
	tdir = ptr + 1;
	for(ptr ++; *ptr != '\0' && *ptr != '"'; ptr ++) ;
	if(*ptr != '"') return -1;
	*ptr = '\0';
	*dir = StrDup(tdir);

	return 0;
};

int
Ftp::size(const char *file, off_t *size)
{
	int ret;
	char *ptr;
	
	if(!file) return -1;

	ret = ftp_cmd("SIZE", file);
	switch(ret){
		case 213:
			break;
		default:
			return ret;
	}

	*size = 0;
	ptr = stateLine;
	while(*ptr == ' ') ptr++;
	while(ISDIGIT(*ptr)){
		*size *= 10;
		*size += *ptr -= '0';
		ptr ++;
	}
	
	return 0;
};

int
Ftp::rest(off_t offset)
{
	int ret;
	if(offset < 0) return -1;
	char buffer[64];

	snprintf(buffer, 64, "%lld", offset);
	ret = ftp_cmd("REST", buffer);
	switch(ret){
		case 350:
			return 0;
		default:
			return ret;
	}
};

int
Ftp::set_mode(int mode)
{
	if(mode != PASV && mode != PORT) return -1;
	this->mode = mode;
	return 0;
};

void
Ftp::set_log( void(*f)(const char*str, ...))
{
	this->log = f;
};

void
Ftp::set_timeout(long timeout)
{
	this->timeout = timeout;
};

int
Ftp::type(const char* type)
{
	int ret;

	if(!type) return -1;
	ret = ftp_cmd("TYPE", type);
	switch(ret){
		case 200:
			return 0;
		default:
			return ret;
	}
};

int
Ftp::pasv(int *port)
{
	int ret;
	char *ptr;
	*port = 0;

	/**********************************
	 * RFC2428
	 * ipv4:
	 * <=PASV 
	 * =>227 xxxx(h1,h2,h3,h4,p1,p2)
	 * ipv6:
	 * normal:
	 * <=EPSV
	 * extension:
	 * <=EPSV 1  //use ipv4 connection
	 * <=EPSV 2  //use ipv6 connection
	 * =>229 xxxx(|||6446|)
	 */

	if(remoteAddr.ai_family == AF_INET){ // ipv4
		ret = ftp_cmd("PASV");
		switch(ret){
			case 227:
				break;
			default:
				return ret;
		}
		// xxxx(h1,h2,h3,h4,p1,p2)
		if((ptr=strrchr(stateLine, ',')) == NULL) return -1;
		*port = atoi(ptr+1);
		do{
			ptr -- ;
		}while(ptr != stateLine && ISDIGIT(*ptr));
		if(ptr == stateLine) return -1;
		*port += atoi(ptr+1)<<8;
	}else if(remoteAddr.ai_family == AF_INET6){
		ret = ftp_cmd("EPSV", "2");
		switch(ret){
			case 229:
				break;
			default:
				return ret;
		}
		// xxxx(|||6446|)
		if((ptr=strrchr(stateLine, '|')) == NULL) return -1;
		do{
			ptr --;
		}while(ptr != stateLine && ISDIGIT(*ptr));
		if(ptr == stateLine) return -1;
		*port = atoi(ptr+1);
	}else
		return -1;

	return 0;
};

int
Ftp::port(int port)
{
	int ret;
	char buffer[128];
	char addr[INET6_ADDRSTRLEN];
	char *ptr;

	/************************************
	 * RFC2428
	 * ipv4:
	 * <=PORT 10,20,12,66,port>>8,port&0xff
	 * <=EPRT |1|132.235.1.2|6275|
	 * ipv6: 
	 * <=EPRT |2|IPv6.ascii|PORT.ascii|
	 * =>200 xxx
	 */
	if(localAddr.ai_family == AF_INET){
		if(localAddr.get_addr(addr, INET_ADDRSTRLEN) < 0)
			return -1;
		ptr = addr; 
		while(*ptr){
			if(*ptr == '.') *ptr = ',';
			ptr ++;
		}
		snprintf(buffer, 128, "%s,%d,%d", addr, port>>8, port&0xff);
		ret = ftp_cmd("PORT", buffer);
	}else if(localAddr.ai_family == AF_INET6){
		if(localAddr.get_addr(addr, INET6_ADDRSTRLEN) < 0)
			return -1;
		snprintf(buffer, 128, "|2|%s|%d|", addr, port);
		ret = ftp_cmd("EPRT", buffer);
	}else
		return -1;

	switch(ret){
		case 200:
			break;
		default:
			return ret;
	}

	return 0;
};

// set command to the server and wating data-connection
// opened successfully
int
Ftp::ftp_data_cmd(const char* cmd, const char* args, off_t offset)
{
	int ret;
	int port;
	TcpSockAddr data_sock;
	TcpAcceptor taptr;

	// initial data-connection
	if(mode == PASV){ //passive
		ret = pasv(&port);
		if(ret != 0) return ret;

		data_sock = remoteAddr;
		data_sock.set_port(port);

		dataConn = TcpConnector::connect(data_sock, ret, timeout);
		if(!dataConn) return ret;
	}else if(mode == PORT){ // port
		data_sock = localAddr;
		data_sock.set_port(0);
		ret = taptr.listen(data_sock);
		if(ret < 0) return ret;
		port = taptr.get_bind_port();

		ret = this->port(port);
		if(ret != 0) return ret;
	}

	if(offset > 0){
		ret = rest(offset);
		if(ret != 0) return ret;
	}
	// send command and wait
	ret = ftp_cmd(cmd, args);
	switch(ret){
		case 150:
		case 125:
			break;
		default:
			return ret;
	}

	if(mode == PORT){
		dataConn = taptr.accept(remoteAddr, ret, timeout);
		if(!dataConn) return ret;
	}

	dataConn->set_tos();
	// data-connecting is opened and can read from 
	// the data-connection now
	return 0;
};

// quit normally
int
Ftp::quit()
{
	int ret;

	ret = ftp_cmd("QUIT");
	switch(ret){
		case 221:
			break;
		default:
			return ret;
	}
	return 0;
};

// get maxsize bytes, will be used by RETR
int
Ftp::read_data(char *buffer, int maxsize)
{
	return dataConn->read(buffer, maxsize, timeout);
};

// get a line, will be used by LIST
int
Ftp::gets_data(char *buffer, int maxsize)
{
	return dataConn->read_line(buffer, maxsize, timeout);
};

int
Ftp::data_ends()
{
	int ret;

	delete dataConn; dataConn = NULL;
	ret = ftp_cmd();
	switch(ret){
		case 226:
		case 250:
			return 0;
		default:
			return ret;
	}
};

int
Ftp::list(const char *dir)
{
	return ftp_data_cmd("LIST", dir);
};

int
Ftp::retr(const char *file, off_t offset)
{
	if(!file) return -1;

	return ftp_data_cmd("RETR", file, offset);
};
