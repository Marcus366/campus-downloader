#ifndef __CAMPUS_DOWNLOADER_H__
#define __CAMPUS_DOWNLOADER_H__

#include <uv.h>
#include "task.h"
#include "http_url.h"
#include "http_request.h"


typedef struct addrinfo addrinfo;

typedef struct downloader {
	task          *task;
	http_url      *url;
	
	addrinfo      *addrinfo;
	
	http_request  *request;

} downloader;


downloader* downloader_new(const char *url, const char *fullname);

int downloader_run(downloader *dler);


#endif