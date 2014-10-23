#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include "campus_downloader.h"


downloader*
downloader_new(const char *url, const char *fullname)
{
	downloader *dler = malloc(sizeof(downloader));

	dler->task = NULL;
	return dler;
}


void
downloader_add_task(downloader *dler, const char *url, const char *fullname)
{
	task *newtask = create_task(url, fullname);
	newtask = dler->task;
}

int
downloader_run(downloader* dler)
{
	return uv_run(uv_default_loop(), UV_RUN_DEFAULT);
}

