#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include "campus_downloader.h"


unsigned downloading = 0;


static int timer_cb(uv_timer_t *handle);


downloader*
downloader_new()
{
	downloader *dler = malloc(sizeof(downloader));

	dler->task = NULL;
	return dler;
}


void
downloader_add_task(downloader *dler, const char *url, const char *fullname)
{
	task *newtask = create_task(dler, url, fullname);
}


int
downloader_run(downloader* dler)
{
	uv_timer_t timer;
	timer.data = dler;
	uv_timer_init(uv_default_loop(), &timer);
	uv_timer_start(&timer, timer_cb, 0, 500);

	return uv_run(uv_default_loop(), UV_RUN_DEFAULT);
}


static int
timer_cb(uv_timer_t *handle)
{
	downloader *dler  = (downloader*)handle->data;
	struct task *task = dler->task;

	if (task == NULL) {
		uv_timer_stop(handle);
		return 0;
	}

	if (downloading == 0) {
		return 0;
	}

	while (task) {
		uint64_t now = uv_now(handle->loop);
		double speed = (task->cur_size - task->last_step_size) * 1.0 / (now - task->last_step_time);

		task->consumed_time += now - task->last_step_time;
		task->last_step_size = task->cur_size;
		task->last_step_time = now;

		printf("download %lld bytes / %lld bytes ================== speed: %.2lfkb/s %.0f%%\r", 
			   task->cur_size, task->total_size, speed,
			   task->cur_size * 100.0f / task->total_size);

		task = task->next;
	}

	return 0;
}