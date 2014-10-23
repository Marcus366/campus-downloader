#ifndef __BLOCK_H__
#define __BLOCK_H__

#include <uv.h>
#include "http_request.h"

typedef struct block {
	struct task  *task;

	http_request *request;

	uint64_t      start;
	uint64_t      end;

	uint64_t      pos;
} block;


block* create_block(struct task *task, uint64_t start, uint64_t end);


void get_block(struct block *block);

#endif