#include <stdlib.h>
#include "worker.h"


static void start_waiting(uv_prepare_t *prepare);


struct worker*
create_worker(uv_thread_cb cb)
{
	struct worker *worker = malloc(sizeof(struct worker));

	worker->cb  = cb;

	worker->loop = malloc(sizeof(uv_loop_t));
	uv_loop_init(worker->loop);

	worker->prepare = malloc(sizeof(uv_prepare_t));
	uv_prepare_init(worker->loop, worker->prepare);
	uv_prepare_start(worker->prepare, start_waiting);
	worker->prepare->data = worker;

	worker->download_list = NULL;
	worker->waiting_list  = NULL;

	worker->next = NULL;

	uv_thread_create(&worker->tid, cb, worker); 

	return worker;
}


void
download_worker_cb(void *arg)
{
	struct worker *worker = (struct worker*)arg;

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
		p = p->next;
	}

	/* link the waiting_list to download_list */
	if (tail != NULL) {
		tail->next = worker->download_list; 

		worker->download_list = worker->waiting_list;
		worker->waiting_list  = NULL;
	}
}