#include <stdlib.h>
#include "worker.h"


static void start_waiting(uv_prepare_t *prepare);


struct worker*
create_worker(uv_thread_cb cb)
{
	struct worker *worker = (struct worker*)malloc(sizeof(struct worker));

	worker->cb  = cb;

	worker->loop = uv_loop_new();

	worker->prepare = NULL;
	worker->async   = NULL;

	worker->download_list = NULL;
	worker->waiting_list  = NULL;

	worker->next = NULL;

	uv_thread_create(&worker->tid, cb, worker);

	return worker;
}


void
cancel_worker(struct worker *worker)
{
  uv_loop_close(worker->loop);
  
}


void
download_worker_cb(void *arg)
{
	struct worker *worker = (struct worker*)arg;

	worker->prepare = (uv_prepare_t*)malloc(sizeof(uv_prepare_t));
	uv_prepare_init(worker->loop, worker->prepare);
	uv_prepare_start(worker->prepare, start_waiting);
	worker->prepare->data = worker;

	worker->async = (uv_async_t*)malloc(sizeof(uv_async_t));
	uv_async_init(worker->loop, worker->async, NULL);
	worker->async->data = worker;

	uv_run(worker->loop, UV_RUN_DEFAULT);
}


void
start_waiting(uv_prepare_t *prepare)
{
	struct worker *worker = (struct worker*)prepare->data;
	struct block *tail = NULL, *p = worker->waiting_list;

	while (p) {
		get_block(p);
		tail = p;
		p = p->worker_next;
	}

	/* link the waiting_list to download_list */
	if (tail != NULL) {
		tail->worker_next = worker->download_list;

		worker->download_list = worker->waiting_list;
		worker->waiting_list  = NULL;
	}
}
