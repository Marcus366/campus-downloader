#ifndef __CAMPUS_DOWNLOADER_H__
#define __CAMPUS_DOWNLOADER_H__

#include <uv.h>
#include "task.h"
#include "http_url.h"
#include "http_request.h"


typedef struct addrinfo addrinfo;

extern unsigned downloading;

typedef struct downloader {
	task          *task;
	
} downloader;


downloader* downloader_new();

void downloader_add_task(downloader *dler, const char *url, const char *fullname);

int downloader_run(downloader *dler);


#endif