#include "task.h"

static int
block_cmp(const void *lhs, const void *rhs)
{
	block *lb = (block*)lhs, *rb = (block*)rhs;
	return lb->start - rb->start;
}


struct task*
create_task(const char *fullname)
{
	struct task *task = malloc(sizeof(struct task));

	task->name     = _strdup(fullname);
	task->cur_size = 0;

	task->blocks   = skiplist_new(16, block_cmp);

	return task;
}