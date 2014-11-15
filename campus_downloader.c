#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include "campus_downloader.h"
#include "block.h"


unsigned downloading = 0;


static void settle(uv_timer_t *handle);
static void discover(uv_timer_t handle);
static void on_alloc(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf);
static void on_discover(uv_udp_t *handle, ssize_t nread, const uv_buf_t *buf, 
                      const struct sockaddr *addr, unsigned flags);


downloader*
downloader_new()
{
	int i;
  struct sockaddr_in addr;

	downloader *dler = (downloader*)malloc(sizeof(downloader));

	dler->mainloop = uv_default_loop();

	dler->tasks = NULL;

	for (i = 0; i < 4; ++i) {
		struct worker* worker = create_worker(download_worker_cb);

		worker->next = dler->workers;
		dler->workers = worker;
	}

  dler->discover = (uv_udp_t*)malloc(sizeof(uv_udp_t));
  dler->discover->data = dler;
  uv_udp_init(dler->mainloop, dler->discover);
  uv_ip4_addr("0.0.0.0", 0, &addr);
  uv_udp_bind(dler->discover, (struct sockaddr*)&addr, 0);
  uv_udp_recv_start(dler->discover, on_alloc, on_discover);

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
	uv_timer_start(&timer, settle, 0, 500);

	return uv_run(dler->mainloop, UV_RUN_DEFAULT);
}


static void
settle(uv_timer_t *handle)
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
		struct block *block = task->blocks;
		while (block != NULL) {
			/* critical area start, the downloaded_pos may be changed by worker thread */
			uint64_t downloaded = block->downloaded_pos - block->confirmed_pos;
			task->cur_size       += downloaded;
			block->confirmed_pos += downloaded;
			/* critical area end */
			block = block->task_next;
		}
		
		uint64_t now = uv_now(handle->loop);
		double speed = (task->cur_size - task->last_step_size) * 1.0 / (now - task->last_step_time);

		task->consumed_time += now - task->last_step_time;
		task->last_step_size = task->cur_size;
		task->last_step_time = now;

		printf("download %lu bytes / %lu bytes ====== speed: %.2lfkb/s %.0f%%\r", 
			   task->cur_size, task->total_size, speed,
			   task->cur_size * 100.0f / task->total_size);

		task = task->next;
	}

}


static void
on_alloc(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf)
{
  (void) handle;
  *buf = uv_buf_init((char*)malloc(suggested_size), suggested_size);
}


static void on_discover(uv_udp_t *handle, ssize_t nread, const uv_buf_t *buf, 
                      const struct sockaddr *addr, unsigned flags)
{
}

