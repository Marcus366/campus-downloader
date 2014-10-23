#ifndef __TASK_H__
#define __TASK_H__

#include <uv.h>

#include "third-party/container/skiplist.h"

typedef struct block {
	struct task *task;

	uint64_t     start;
	uint64_t     end;

	uint64_t     pos;
} block;


typedef struct task {
	uv_file     fd;
	const char *name;

	uint64_t    cur_size;
	uint64_t    total_size;

	skiplist   *blocks;

} task;


struct task* create_task(const char *fullname);

#endif