#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "campus_downloader.h"

int
main(int argc, char** argv)
{
	char *str, *filename;
	//const char *str = "http://www.baidu.com";
	//const char *str = "http://blog.csdn.net/mazhaojuan/article/details/6978065";
	str = "http://github-windows.s3.amazonaws.com/GitHubSetup.exe";
	//str = "http://www.shuming.cc/modules/article/packdown.php?id=3&type=txt&fname=%CE%D2%B5%C4%C7%E9%C8%CB%CE%D2%B5%C4%B0%AE%C8%CB";
	filename = strrchr(str, '/') + 1;

	if (argc == 2) {
		str = argv[1];
		filename = strrchr(str, '/') + 1;
	} else if (argc == 3) {
		str = argv[1];
		filename = argv[2];
	}

	downloader *dler = downloader_new(str, filename);
	downloader_run(dler);

	system("pause");
	return 0;
}