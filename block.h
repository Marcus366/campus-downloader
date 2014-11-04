#ifndef __BLOCK_H__
#define __BLOCK_H__

#include <uv.h>
#include "http_request.h"

typedef struct block {
	struct task   *task;
	struct worker *worker;

	http_request  *request;

	uint64_t       start;
	uint64_t       end;

	uint64_t       downloaded_pos;
	uint64_t       confirmed_pos;

	struct block  *task_next;
	struct block  *worker_next;
} block;


block* create_block(struct task *task, uint64_t start, uint64_t end);
void   dispatcher_block(struct task* task, struct block *block);


void get_block(struct block *block);

#endif
