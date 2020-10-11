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


#ifndef _FTPPARSER_H
#define _FTPPARSER_H

#include <sys/types.h>

class FtpParser
{
	public:
		FtpParser();
		~FtpParser();

		const char *get_file(void);
		const char *get_link(void);
		off_t get_size(void);
		time_t get_time(void);
		char get_type(void);

		int parse(const char *line);

	private:
		void reset(void);
		int process_dos(char *line);
		int process_unix(char *line);

	private:
		const char *file;
		const char *link;
		off_t size;
		time_t time;
		char type;
};

#endif // _FTPPARSER_H
