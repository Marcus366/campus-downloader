#ifndef __WORKER_H__
#define __WORKER_H__

#include <uv.h>
#include "block.h"

typedef struct worker {
	uv_thread_t    tid;
	uv_thread_cb   cb;

	uv_loop_t     *loop;

	block         *download_list;
	block         *waiting_list;

	uv_prepare_t  *prepare;

	struct worker *next;
} worker;


struct worker* create_worker(uv_thread_cb cb);

void download_worker_cb(void *arg);

#endif