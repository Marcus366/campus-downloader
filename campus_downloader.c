#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include "campus_downloader.h"
#include "block.h"


unsigned downloading = 0;


static void timer_cb(uv_timer_t *handle);


downloader*
downloader_new()
{
	int i;
	downloader *dler = (downloader*)malloc(sizeof(downloader));

	dler->mainloop = uv_default_loop();

	dler->tasks = NULL;

	for (i = 0; i < 4; ++i) {
		struct worker* worker = create_worker(download_worker_cb);

		worker->next = dler->workers;
		dler->workers = worker->next;
	}

	return dler;
}


void
downloader_add_task(downloader *dler, const char *url, const char *fullname)
{
	create_task(dler, url, fullname);
}


int
downloader_run(downloader* dler)
{
	uv_timer_t timer;
	timer.data = dler;
	uv_timer_init(dler->mainloop, &timer);
	uv_timer_start(&timer, timer_cb, 0, 500);

	return uv_run(dler->mainloop, UV_RUN_DEFAULT);
}


static void
timer_cb(uv_timer_t *handle)
{
	downloader *dler  = (downloader*)handle->data;
	struct task *task = dler->tasks;

	if (task == NULL) {
		uv_timer_stop(handle);
		return;
	}

	if (downloading == 0) {
		return;
	}

	while (task) {
		skipnode *node = task->blocks->header[0]->next[0];
		while (node != task->blocks->header[0]) {
			struct block *block = (struct block*)node->val;
			/* critical area start, the downloaded_pos may be changed by worker thread */
			uint64_t downloaded = block->downloaded_pos - block->confirmed_pos;
			task->cur_size       += downloaded;
			block->confirmed_pos += downloaded;
			/* critical area end */
			node = node->next[0];
		}
		
		uint64_t now = uv_now(handle->loop);
		double speed = (task->cur_size - task->last_step_size) * 1.0 / (now - task->last_step_time);

		task->consumed_time += now - task->last_step_time;
		task->last_step_size = task->cur_size;
		task->last_step_time = now;

		printf("download %ulld bytes / %ulld bytes ================== speed: %.2lfkb/s %.0f%%\r", 
			   task->cur_size, task->total_size, speed,
			   task->cur_size * 100.0f / task->total_size);

		task = task->next;
	}

}
