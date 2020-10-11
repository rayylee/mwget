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

#include "url.h"
#include "proxy.h"
#include "utils.h"

Proxy::Proxy()
{
	type = NONE_PROXY;
	host = NULL;
	user = NULL;
	password = NULL;
};

Proxy::~Proxy()
{
	delete[] host;
	delete[] user;
	delete[] password;
};

ProxyType
Proxy::get_type(void)
{
	return type;
};

const char*
Proxy::get_host(void)
{
	return host;
};

int
Proxy::get_port(void)
{
	return port;
};

const char*
Proxy::get_user(void)
{
	return user;
};

const char*
Proxy::get_password(void)
{
	return password;
};

void
Proxy::set_host(const char *host)
{
	delete[] this->host;
	this->host = StrDup(host);
};

void
Proxy::set_user(const char *user)
{
	delete[] this->user;
	this->user = StrDup(user);
};

void
Proxy::set_password(const char *password)
{
	delete[] this->password;
	this->password = StrDup(password);
};
	
void
Proxy::set_port(int port)
{
	this->port = port;
};

void
Proxy::set_type(ProxyType type)
{
	this->type = type;
};

Proxy&
Proxy::operator = (Proxy &proxy)
{
	if(this != &proxy){
		delete[] host;
		delete[] user;
		delete[] password;
		type = proxy.get_type();
		port = proxy.get_port();
		host = StrDup(proxy.get_host());
		user = StrDup(proxy.get_user());
		password = StrDup(proxy.get_password());
	}

	return *this;
};
