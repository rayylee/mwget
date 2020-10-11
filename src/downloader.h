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


#ifndef _DOWNLOADER_H
#define _DOWNLOADER_H

#include <iostream>

#include "plugin.h"
#include "task.h"
#include "block.h"
#include "progressbar.h"

using namespace std;

class Downloader
{
	public:
		Downloader(void);
		~Downloader(void);

		int run(void);

	public:
		Task task;

	private:
		int init_plugin(void);
		int init_task(void);
		int init_local_file_name(void);
		int init_threads_from_mg(void);
		int init_threads_from_info(void);

		int thread_create(void);
		int self(void);
		int schedule(void);
		int save_temp_file_exit(void);
		static int download_thread(Downloader *downloader);
		int directory_download(void);
		int file_download(void);

	private:
		Plugin *plugin;
		char *localPath;
		char *localMg;
		int threadNum;
		Block *blocks;
		ProgressBar *pb;
};

#endif // _DOWNLOADER_H
