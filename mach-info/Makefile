all:config.h
config.h:
	echo "#define BUILT_TIME \"`date +%Y-%m-%d` `date +%H:%M:%S`\"">> config.h
	echo "#define CC_VERSION \"`LANG=C cc --version |grep cc`\"">> config.h
	echo "#define CXX_VERSION \"`LANG=C c++ --version |grep c++`\"">> config.h
	echo "#define SYSTEM_TYPE \"`LANG=C uname -o` `LANG=C uname -m`\"" >> config.h
clean:
	rm -f config.h
distclean:
	rm -f config.h
install:
	echo 'There are configure directory,No files to install!'
#死TM Auto{make,conf}！害得我除此下策
#make install的时候还会出问题！
