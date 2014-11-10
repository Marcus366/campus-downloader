#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "campus_downloader.h"

int
main(int argc, char** argv)
{
	const char *str, *filename;
	str = "http://github-windows.s3.amazonaws.com/GitHubSetup.exe";
	//str = "http://127.0.0.1/jdk-7u17-windows-x64.exe";
	filename = strrchr(str, '/') + 1;

	if (argc == 2) {
		str = argv[1];
		filename = strrchr(str, '/') + 1;
	} else if (argc == 3) {
		str = argv[1];
		filename = argv[2];
	}

	downloader *dler = downloader_new();
	downloader_add_task(dler, str, filename);
	downloader_run(dler);

  // never come here.
  // make valgrind happy.

	return 0;
}
