/*  MWget - A Multi download for all POSIX systems.
 *  Homepage: http://mwget.sf.net
 *  Copyright (C) 2005- rgwan,xiaosuo
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
#include <getopt.h>
#include <signal.h>

#include "mwget.h"

using namespace std;
#define BUGREPORTEMAIL "<sa@kmlinux.tk><xiao_suo@hotmail.com>"
#define BUGREPORTURL "<http://mwget.sourceforge.net/> <http://www.kmlinux.tk/>"
void
print_help()
{


	//cout<<"mwget "VERSION,_(": A multi progress command - downloader of all POSIX Systems.")<<endl;
	printf("GNU MWget %s %s\n",VERSION,_(",a non-interactive and multiline network retriever of all POSTIX Systems."));
	//cout<<_("Usage: ")"mwget"_(" [options]... [URL]...")<<endl; // 偶讨厌cout，特别是加了Gettext的cout，简直没法写了～！
	printf("%s mwget %s\n",_("Usage: "),_(" [Options]... [URL]..."));
	
	cout<<_("Options:")<<endl;
	cout<<_("  -b,  --debug          Show the debug message")<<endl;
	cout<<_("  -c,  --count=num      Set the retry count to [num], no limit when \"0\", the default is \"99\"")<<endl;
	cout<<_("  -d,  --directory=dir  Set the local direcotry to [dir], the default is \".\"")<<endl;
	cout<<_("  -f,  --file=file      Rename the file to [file]")<<endl;
	cout<<_("  -h,  --help           A brief summary of all the options")<<endl;
	cout<<_("  -i,  --interval=num   Set the ftp retry interval to [num] seconds, the default is \"5\"")<<endl;
	cout<<_("  -n,  --number=num     Use [num] connections instead of the default (4)")<<endl;
	cout<<_("  -r,  --referer=URL    Include `Referer: [URL]\' header in HTTP request.")<<endl;
	cout<<_("  -t,  --timeout=num    Set the connection timeout to [num] seconds, the default is \"30\"")<<endl;
	cout<<_("  -v,  --version        Show the version of the mwget and exit")<<endl;
	cout<<_("  -x,  --proxy=URL      Set the proxy [URL]")<<endl;
	printf("\n\n%s%s\n%s%s\n",_("Mail bug reports and suggestions to "),BUGREPORTEMAIL,_("On website bug reports and suggestions to "),BUGREPORTURL);
	printf("%s\n",_("We Welcome your BUG REPORT!"));
};
void print_missing()
{
printf("mwget: %s\n",_("missing URL"));
printf("%s mwget %s\n\n",_("Usage: "),_(" [Options]... [URL]..."));
printf("%s `mwget --help` %s\n",_("Try"),_("for more options,Thanks!"));

}
void print_version()
{
printf("MWget %s %s\n\n",_("Version"),VERSION);
printf("%s\n",_("Machine Information of MWget:"));
printf("  %s %s\n",_("Built time on:"),BUILT_TIME);
printf("  %s \"%s\"\n",_("C Compiler Version:"),CC_VERSION);
printf("  %s \"%s\"\n",_("C++ Compiler Version:"),CXX_VERSION);
printf("  %s \"%s\"\n\n",_("Built host type:"),SYSTEM_TYPE);
printf("%s\n",_("Misc Informaton for MWget:"));
printf("  %s\n",_("Based on myget 0.1.2,xiaosuo."));
printf("  %s\n",_("It 's a branch of myget."));
printf("  %s\n",_("It 's name is \"Multi line\" wget."));
printf("  %s\n",_("So I give a name for this program:MWget."));
printf("  %s\n",_("Maintainer:rgwan(WanZhiYuan),Email:<sa@kmlinux.tk>"));
printf("  %s\n",_("Project home:<http://mwget.sourceforge.net>"));
}
/*
void print_moo()
{
printf("         (__) \n");
printf("         (oo) \n");
printf("   /------\\/  \n");
printf("  / |    ||   \n");
printf(" *  /\\---/\\ \n");
printf("    ~~   ~~   \n");
printf("....\"%s\"...",_("Have you mooed today?"));
}
*/
const struct option long_options [] = {
	{"debug", 0, NULL, 'b'},
	{"count", 1, NULL, 'c'},
	{"direcotry", 1, NULL, 'd'},
	{"file", 1, NULL, 'f'},
	{"help", 0, NULL, 'h'},
	{"interval", 1, NULL, 'i'},
	{"number", 1, NULL, 'n'},
	{"referer", 1, NULL, 'r'},
	{"timeout", 1, NULL, 't'},
	{"version", 0, NULL, 'v'},
	{"proxy", 1, NULL, 'x'},
	//{"moo", 1, NULL, 'm'},
	{NULL, 0, NULL, 0}
};

char short_options [] = "bc:d:f:hi:n:r:t:vx:";

int
main(int argc, char **argv)
{
	int ret;
	URL url;
	Downloader downloader;
	Task task;
	Proxy proxy;
	char *ptr = NULL;
	// init_config
	// parse arguments
	// parse url
	// ftp or http ?
	// new download thread
	seti18npackage();//设置国际化
	signal(SIGPIPE, SIG_IGN);
#ifdef HAVE_SSL
	SSL_load_error_strings();
	SSLeay_add_ssl_algorithms();
#endif
	
	while(1){
		int option_index = 0;
		//if(argv[1]=="moo")
		//{
		//	printf("moo\n");
		//	print_moo();//超级牛力
		//	exit(0);
		//}
		//printf("%s",argv[1]);
		ret = getopt_long(argc, argv, short_options,
				long_options, &option_index);
		if(ret == -1) break;
		switch(ret){
			case 'b':
				global_debug = true;
				break;
			case 'c':
				task.tryCount = atoi(optarg);
				break;
			case 'd':
				task.set_local_dir(optarg);
				break;
			case 'f':
				task.set_local_file(optarg);
				break;
			case 'h':
				print_help();
				return 0;
				break;
			case 'i':
				task.retryInterval = atoi(optarg);
				break;
			case 'n':
				task.threadNum = atoi(optarg);
				break;
			case 'r':
				task.set_referer(optarg);
				break;
			case 't':
				task.timeout = atoi(optarg);
				break;
			case 'v':
				print_version();
				return 0;
			case 'x':
				ptr = StrDup(optarg);
				break;
			/*case 'm':
				print_moo();//超级牛力
				return 0;
				break;*/
			case '?':
			default:
				//print_help();
				print_missing();
				return -1;
		}
	}
	
	if(ptr == NULL){
		ptr = StrDup(getenv("proxy"));
	}
	if(ptr){
		if(url.set_url(ptr) < 0){
			delete[] ptr;
			cerr<<_("!!!Please check your http_proxy setting!")<<endl;
			print_help();
			return -1;
		}
		delete[] ptr;
		if(url.get_protocol() != HTTP){
			cerr<<_("!!!The proxy type is not supported")<<endl;
			return -1;
		}
		proxy.set_type(HTTP_PROXY);
		proxy.set_host(url.get_host());
		proxy.set_port(url.get_port());
		proxy.set_user(url.get_user());
		proxy.set_password(url.get_password());
		task.proxy = proxy;
	}

	
	if(optind >= argc){
		//print_help();
		print_missing();
		return -1;
	}

	while(optind < argc){
		if(url.set_url(argv[optind++]) < 0){
			print_help();
			return -1;
		}
		task.url = url;
		downloader.task = task;
		downloader.run();
	}

	return 0;
};
