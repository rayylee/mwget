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


#ifndef _PROGRESSBAR_H
#define _PROGRESSBAR_H

#include <sys/types.h>
#include <unistd.h>

#define MINIMUM_SCREEN_WIDTH 45 // the minimum screen width 
#define DEFAULT_SCREEN_WIDTH 80 // the default screen width when can not get the screen width
#define MAXIMUM_SCREEN_WIDTH 256 //the maximum screen width
#define DIRTY_WIDTH 35

class ProgressBar
{
	public:
		ProgressBar(off_t total_size = 0, int block_num = 1);
		~ProgressBar();

		void set_start_point(off_t *start_point);
		void set_total_size(off_t size){ totalSize = size; };
		void set_block_num(int num);
		void update(off_t *data);
		void init(void);

	private:
		static void screen_width_change(int signo);
		
	private:
		double lastTime; // the last time of update
		off_t lastDownloaded; // the total size of the already download part
		off_t totalSize; // the total size of the file in bytes
		off_t *startPoint; // save the start points
		int blockNum; // blockNum
		bool show; // can show or not

		float rates[12];
		int rateIndex;
		int rateCount;

		long percent;
		char downloaded[5];
		char downloadRate[5];
		char eta[6];
		char graph[MAXIMUM_SCREEN_WIDTH - DIRTY_WIDTH + 1];
		static int graphWidth;
		int direction;
};

#endif // _PROGRESSBAR_H
