#ifndef __TASK_H__
#define __TASK_H__

#include <uv.h>

#include "http_url.h"
#include "http_request.h"
#include "third-party/container/skiplist.h"


/*
 * A task is mean a download task. In this downloader,
 * one task is just associated with one downloading file.
 */
typedef struct task {
	struct downloader *dler;

	/* attached file message */
	uv_file            fd;
	const char        *name;

	/* server message */
	http_url          *url;
	struct addrinfo   *addrinfo;

	http_request      *head_request;

	/* */
	uint64_t           cur_size;
	uint64_t           total_size;
	struct block      *blocks;

	/* time statistic */
	uint64_t           start_time;
	uint64_t           consumed_time;

	/* used for calculate speed */
	uint64_t           last_step_time;
	uint64_t           last_step_size;

	/* a list to link all the task */
	struct task       *next;
} task;


struct task* create_task(struct downloader *dler, const char *url, const char *fullname);



#endif