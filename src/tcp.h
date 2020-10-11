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

// support ipv4 and ipv6
/* getaddrinfo is a thread-safe function in linux system
 * so these classes are thread-safe too*/
#ifndef _TCP_H
#define _TCP_H

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <netinet/ip.h>

#include "advio.h"
/***
 * store the tcp socket address information
 * support ipv4 and ipv6 */
class TcpSockAddr
{
	public:
		/* default ipv4 */
		TcpSockAddr(int family=AF_INET);
		void set_family(int family);
		// ~TcpSockAddr();
		// TcpSockAddr(const TcpSockAddr& sock);
		// void operator = (const TcpSockAddr& sock);
		// set the tcp port
		void set_port(int port);
		int get_port();
		// set addr 
		int set_addr(const char *addr);
		// get the address ipv4 or ipv6
		int get_addr(char *addr, int size);

	public:
		int ai_family;
		char ai_addr[24]; // ipv6's length is 24 bytes
		socklen_t ai_addrlen;
};

class TcpConnection
	: public BufferStream
{
	public:
		TcpConnection():BufferStream(-1){};
		// ~TcpConnection();
		// TcpConnection(const TcpConnection &con);
		// void operator = (const TcpConnection &con);

		int set_tos(void);
		bool is_connected(); // try to judge the connection
		int get_remote_addr(TcpSockAddr& sockaddr)const; // return this remote
		int get_local_addr(TcpSockAddr& sockaddr)const; // return local
};

class Address
{
	public:
		Address(){ addr = NULL; };
		~Address(){ freeaddrinfo(addr);	};
		// resolve dns_name with getaddrinfo
		int resolve(const char *dns_name, int port, int family=AF_UNSPEC);
	public:
		struct addrinfo *addr;
};

class TcpConnector
{
	public:
		static TcpConnection* connect(const Address& addr, int& ret,  long timeout=-1);
		static TcpConnection* connect(const TcpSockAddr& addr, int& ret,  long timeout=-1);
};

class TcpAcceptor
{
	public:
		TcpAcceptor(){ listen_fd = -1; };
		~TcpAcceptor(){ close(listen_fd); };
		// forbid the follow two functions
		// TcpAcceptor(const TcpAcceptor& acc);
		// void operator = (const TcpAcceptor& acc);

		// if bind port == 0, when binded it will return a random port */
		int get_bind_port();
		int listen(const TcpSockAddr& local, int backlog=1);
		TcpConnection* accept(int &ret, long timeout=-1);
		// wait expect's connection
		TcpConnection* accept(const TcpSockAddr& expect, int &ret, long timeout=-1);
	private:
		int listen_fd;
};

#endif // _TCP_H
