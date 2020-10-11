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

#include <signal.h>
#include <errno.h>
#include <sys/stat.h>//PATCH
#include <sys/types.h>

#include "downloader.h"
#include "macro.h"
#include "utils.h"
#include "ftpplugin.h"
#include "httpplugin.h"
#include "progressbar.h"
#include "debug.h"

typedef void* (*PthreadFunction) (void*);

bool global_sigint_received = false;
bool global_downloading = false;

void
catch_ctrl_c(int signo)
{
	if(global_downloading){
		global_sigint_received = true;
	}else{
		pthread_exit(0);
	}
};

Downloader::Downloader(void)
{
	plugin = NULL;
	blocks = NULL;
	localPath = NULL;
	localMg = NULL;
	pb = new ProgressBar;

	signal(SIGPIPE, SIG_IGN);
	signal(SIGINT, &catch_ctrl_c);
};

Downloader::~Downloader(void)
{
	delete[] blocks;
	delete[] localPath;
	delete[] localMg;
	delete pb;
	delete plugin;

	signal(SIGPIPE, SIG_DFL);
	signal(SIGINT, SIG_DFL);
};

int
Downloader::init_plugin(void)
{
	if(task.proxy.get_type() == HTTP_PROXY){
		delete plugin;
		plugin = new HttpPlugin;
	}else{
		switch(task.url.get_protocol()){
			case HTTP:
#ifdef HAVE_SSL
			case HTTPS:
#endif
				delete plugin;
				plugin = new HttpPlugin;
				break;
			case FTP:
				delete plugin;
				plugin = new FtpPlugin;
				break;
			default:
				return -1;
		}
	}

	return 0;
};

int
Downloader::init_task(void)
{
	int i;
	int ret;

_reinit_plugin:
	if(init_plugin() < 0){
		cerr<<_("Unknown protocol")<<endl;
		return -1;
	}

	for(i = 0; task.tryCount <= 0 || i < task.tryCount; i ++){
		ret = plugin->get_info(&task);
		if(ret == -1){
			return -1;
		}else if(ret == S_REDIRECT){
			cerr<<_("Redirect to: ")<<task.url.get_url()<<endl;
			goto _reinit_plugin;
		}else if(ret == 0){
			return 0;
		}else{
			continue;
		}
	}

	return E_MAX_COUNT;
};

int
Downloader::init_local_file_name(void)
{
	int length;
	char *tmpStr;

	length = task.get_local_dir() ? strlen(task.get_local_dir()) : 1;
	length += task.get_local_file() ? strlen(task.get_local_file()) :
		strlen(task.url.get_file());
	length += 6;

	tmpStr = new char[length];

	snprintf( tmpStr, length, "%s/%s.mg!",
			task.get_local_dir() ? task.get_local_dir() : ".",
			task.get_local_file() ? task.get_local_file() :
				task.url.get_file() );
	delete[] localPath;
	delete[] localMg;
	tmpStr[length - 5] = '\0';
	localPath = StrDup(tmpStr);
	tmpStr[length - 5] = '.';
	localMg = tmpStr;

	return 0;
};

int
Downloader::init_threads_from_mg(void)
{
	FILE *fd;
	int i;
	struct stat file_stat;

	if(stat(localMg, &file_stat) < 0){
		perror(_("Can not get the info of the temp file"));
		return -1;
	}
	if(file_stat.st_size < task.fileSize + sizeof(threadNum)){
		cerr<<_("the temp file: \"")<<localMg<<_("\" is not correct")<<endl;
		return -1;
	}
	fd = fopen(localMg, "r");
	if(fd == NULL && errno == EACCES){
		cerr<<_("Can not access the temp file: ")<<localMg<<endl;
		return -1;
	}

	fseeko(fd, task.fileSize, SEEK_CUR);
	fread(&threadNum, sizeof(threadNum), 1, fd);
	if(file_stat.st_size != task.fileSize + sizeof(threadNum) + sizeof(off_t)*threadNum*3){
		cerr<<_("the temp file: \"")<<localMg<<_("\" is not correct")<<endl;
		fclose(fd);
		return -1;
	}
	
	delete[] blocks;
	blocks = new Block[threadNum];
	for(i = 0; i < threadNum; i ++){
		fread(&blocks[i].startPoint, sizeof(off_t), 1, fd);
		fread(&blocks[i].downloaded, sizeof(off_t), 1, fd);
		fread(&blocks[i].size, sizeof(off_t), 1, fd);
		if(blocks[i].bufferFile.open(localMg) < 0){
			perror(_("Can not open the temp file to write"));
			return -1;
		}
	}

	fclose(fd);
	return 0;
};

int
Downloader::init_threads_from_info(void)
{
	off_t block_size;
	int i;

	threadNum = task.threadNum > 0 ? task.threadNum : 1;
	block_size = task.fileSize / threadNum;
	if(block_size <= 0){ // too small file
		threadNum = 1;
		block_size = task.fileSize;
	}

	delete[] blocks;
	blocks = new Block[threadNum];
	for(i = 0; i < threadNum; i ++){
		blocks[i].startPoint = i * block_size;
		blocks[i].size = block_size;
		if(blocks[i].bufferFile.open(localMg) < 0){
			perror(_("Can not open the temp file to write"));
			return -1;
		}
	}

	blocks[threadNum - 1].size = task.fileSize - block_size * ( threadNum - 1);

	return 0;
};

int
Downloader::thread_create(void)
{
	pthread_t pid;
	int i;

	while(1){
		i = pthread_create(&pid, NULL, 
				(PthreadFunction)&download_thread, (void*)this);
		if(i == 0) break;
		usleep(250000);
	}

	for(i = 0; i < threadNum; i ++){
		if(blocks[i].pid == 0){ // found an empty slot
			blocks[i].pid = pid;
			break;
		}
	}

	if(i == threadNum) return -1;

	return 0;
};

// return the source'id of the current thread
int
Downloader::self(void)
{
	pthread_t self;
	self = pthread_self();
	int i;
	while(1){
		for(i = 0; i < threadNum; i ++){
			if(blocks[i].pid == self) return i;
		}
		// the parent thread maybe slower than me
	}
};

// this function will be called by pthread_create, because c++
// not allow non-static function convert to the right function,
// so make it static, but this will be safe
int
Downloader::download_thread(Downloader *downloader)
{
	int self, ret, i;
	self = downloader->self();

	/*
	for(i = 0; downloader->task.tryCount <= 0 ||
			i < downloader->task.tryCount; i ++){
			*/
	while(1){
		ret = downloader->plugin->download(downloader->task, downloader->blocks + self);
		if(ret == E_SYS){ // system error
			downloader->blocks[self].state = EXIT;
			return -1;
		}else if(ret == 0){
			downloader->blocks[self].state = EXIT;
			return 0;
		}else{
			continue;
		}
	}

	downloader->blocks[self].state = EXIT;
	return E_MAX_COUNT;
};

// schedule the thread, when the thread exit joined it or wake up it if
// the next thread have not begun to downloading from the serve,
// return the number of the running threads
int
Downloader::schedule(void)
{
	int i, j;
	int joined;

	joined = 0;
	for(i = 0; i < threadNum; i ++){
		if(blocks[i].state == WAIT){
			for(j = i + 1; j < threadNum; j ++){
				if(blocks[i].startPoint + blocks[i].size == blocks[j].startPoint){
					break;
				}
			}
			if(j < threadNum && blocks[j].downloaded == 0){
				if(blocks[j].state == STOP || blocks[j].state == EXIT){
					pthread_cancel(blocks[j].pid);
					pthread_join(blocks[j].pid, NULL);
					blocks[j].state = JOINED;
					blocks[j].bufferFile.close();
				}else if(blocks[j].state != JOINED){
					continue;
				}
				blocks[i].size += blocks[j].size;
				blocks[i].state = WAKEUP;
				blocks[j].startPoint = -1;
				blocks[j].downloaded = 0;
				blocks[j].size = 0;
				i = j;
				joined += j - i;
				off_t *data = new off_t[threadNum];
				for(j = 0; j < threadNum; j ++){
					data[j] = blocks[j].startPoint;
				}
				pb->set_start_point(data);
				delete[] data;
			}
		}else if(blocks[i].state == EXIT){
			pthread_join(blocks[i].pid, NULL);
			blocks[i].state = JOINED;
			blocks[i].bufferFile.close();
			joined ++;
		}else if(blocks[i].state == JOINED){
			joined ++;
		}else{
			continue;
		}
	}

	return threadNum - joined;
}; // end of schedule

int
Downloader::save_temp_file_exit(void)
{
	int i;
	FILE *fd;

	for(i = 0; i < threadNum; i ++){
		if(blocks[i].state != JOINED){
			pthread_cancel(blocks[i].pid);
			pthread_join(blocks[i].pid, NULL);
			blocks[i].bufferFile.close();
		}
	};

	// if know the filesize, maybe can resume through other link address
	// but if the filesize is unknown, it seems impossible
	if(task.fileSize < 0){
		cerr<<"!!!You can not continue in further"<<endl;
		global_downloading = false;
		pthread_exit((void*)1);
	}

	fd = fopen(localMg, "r+");
	fseeko(fd, task.fileSize, SEEK_CUR);
	fwrite(&threadNum, sizeof(threadNum), 1, fd);
	for(i = 0; i < threadNum; i ++){
		fwrite(&blocks[i].startPoint, sizeof(off_t), 1, fd);
		fwrite(&blocks[i].downloaded, sizeof(off_t), 1, fd);
		fwrite(&blocks[i].size, sizeof(off_t), 1, fd);
	}
	fclose(fd);

	global_downloading = false;
	pthread_exit(0);
}; // end of save_temp_file_exit

int
Downloader::directory_download(void)
{
	char tempfile[17];
	int ret;

	strcpy(tempfile, "/tmp/list.XXXXXX");
	ret = mkstemp(tempfile);
	if(ret < 0){
		return -1;
	}
	close(ret);
	debug_log("Tempfile: %s", tempfile);

	if(plugin->get_dir_list(task, tempfile) < 0){
		unlink(tempfile);
		return -1;
	}

	FILE *fd;
	char buf[1024];
	char buf2[1024];
	char *local_dir = NULL;
	int orig_dir_len = 0;
	char *ptr;

	ptr = (char*)task.url.get_dir();
	if(ptr != NULL){
		ptr = strrchr(ptr, '/') ? (strrchr(ptr, '/') + 1) : ptr;
	}
	ptr = task.get_local_file() ? (char*)task.get_local_file() : ptr;
	snprintf(buf, 1024, "%s%s%s",
			task.get_local_dir() ? task.get_local_dir() : ".",
			ptr ? "/" : "",
			ptr ? ptr : "");
	if(mkdir(buf, 00755) < 0 && errno != EEXIST){
		cerr<<_("Can not create directory : ")<<buf<<endl;
		goto _dd_error;
	}
	local_dir = StrDup(buf);
	if(task.url.get_dir()){
		orig_dir_len = strlen(task.url.get_dir());
	}
	task.set_local_file(NULL);

	fd = fopen(tempfile, "r");
	task.isDirectory = false;
	while(1){
		if(fread(&task.fileSize, sizeof(off_t), 1, fd) != 1) break;
		if(fgets(buf, 1024, fd) == NULL) break;
		buf[strlen(buf) - 1] = '\0';
		if(buf[0] == '/'){ // a directory
			snprintf(buf2, 1024, "%s%s",
					local_dir ? local_dir : ".",
					orig_dir_len ? buf + orig_dir_len + 1 : buf);
			if(mkdir(buf2, 00755) < 0 && errno != EEXIST){
				cerr<<_("Can not create directory : ")<<buf2<<endl;
				goto _dd_error;
			}
		}else{ // a file
			cout<<_("Download file : ")<<buf<<endl;
			snprintf(buf2, 1024, "/%s", buf);
			task.url.reset_url(buf2);
			ptr = strrchr(buf, '/');
			if(ptr) *ptr = '\0';
			snprintf(buf2, 1024, "%s%s%s",
					local_dir ? local_dir : ".",
					orig_dir_len ? "" : "/",
					orig_dir_len ? buf + orig_dir_len : buf);
			task.set_local_dir(buf2);
			file_download();
		}
	}

	delete[] local_dir;
	unlink(tempfile);
	return 0;

_dd_error:
	delete[] local_dir;
	unlink(tempfile);
	return -1;
}; // end of directory_download

int
Downloader::file_download(void)
{
	int i;
	int ret = 0;

	init_local_file_name();
	if(file_exist(localPath)){
		cout<<_("File already exist: ")<<localPath<<endl;
		return 0;
	}
	cout<<_("Begin to download: ")
		<<(task.get_local_file() ? task.get_local_file() : task.url.get_file())<<endl;
	char buf[6];
	double time = get_current_time();
	convert_size(buf, task.fileSize);
	cout<<_("FileSize: ")<<buf<<endl;

	if(task.fileSize == 0){
		int fd;
		fd = creat(localPath, 00644);
		if(fd < 0){
			perror(_("Error when I create the file"));
			return -1;
		}else{
			close(fd);
			return 0;
		}
	}

	if(!task.resumeSupported || task.fileSize < 0){
		threadNum = 1;
		delete[] blocks;
		blocks = new Block[1];
		blocks[0].size = task.fileSize;
		blocks[0].bufferFile.open(localMg);
	}else if(file_exist(localMg)){
		ret = init_threads_from_mg();
	}else{
		ret = init_threads_from_info();
	}
	if(ret < 0){
		cerr<<_("Init threads failed")<<endl;
		return ret;
	}

	for(i = 0; i < threadNum; i ++){
		if(thread_create() < 0){
			perror(_("Create thread failed"));
			return -1;
		}
	}

	off_t *data;
	data = new off_t[threadNum];

	for(i = 0; i < threadNum; i ++){
		data[i] = blocks[i].startPoint;
	}
	pb->init();
	pb->set_total_size(task.fileSize);
	pb->set_block_num(threadNum);
	pb->set_start_point(data);

	// update loop
	global_downloading = true;
	while(1){
		if(global_sigint_received){
			delete[] data;
			save_temp_file_exit();
		}

		for(i = 0; i < threadNum; i ++){
			data[i] = blocks[i].downloaded;
		}
		pb->update(data);

		if(schedule() == 0){
			break; // all the thread are exit
		}
		usleep(250000);
	}

	delete[] data;
	// recheck the size of the file if possible
	if(task.fileSize >= 0){
		off_t downloaded;
		downloaded = 0;
		for(i = 0; i < threadNum; i ++){
			downloaded += blocks[i].downloaded;
		}
		// the downloaded maybe bigger than the filesize
		// because the overlay of the data
		if(downloaded < task.fileSize){
			cerr<<_("!!!Some error happend when downloaded")<<endl;
			cerr<<_("!!!So Redownloading is recommended")<<endl;
			save_temp_file_exit();
		}
		truncate(localMg, task.fileSize);
	}
	
	if(rename(localMg, localPath) < 0){
		perror(_("Rename failed"));
		return -1;
	}
	global_downloading = false;

	time = get_current_time() - time;
	convert_time(buf, time);
	cout<<_("Download successfully in ")<<buf<<_(" M:S")<<endl;

	return 0;
}; // end of file_download

int
Downloader::run(void)
{
	int ret;

	ret = init_task();
	if(ret < 0){
		cerr<<_("Can not get the info of the file ")<<endl;
		return ret;
	}

	if(task.isDirectory){
		cerr<<_("This is a directory: ")<<task.url.get_url()<<endl;
		return directory_download();
	}

	return file_download();
}; // end of run
