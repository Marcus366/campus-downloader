#include <stdio.h>
#include "campus_downloader.h"
#include "block.h"
#include "task.h"


extern unsigned downloading;


static void send_request(uv_connect_t *req, int status);
static void on_alloc(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf);
static void on_read(uv_stream_t* stream, ssize_t nread, const uv_buf_t *buf);

static int on_body(http_parser *parser, const char *at, size_t length);
static int on_message_complete(http_parser *parser);


struct block*
create_block(struct task *task, uint64_t start, uint64_t end)
{
	struct block *block = malloc(sizeof(block));

	block->task  = task;
	block->start = start;
	block->end   = end;

	block->downloaded_pos = start;
	block->confirmed_pos  = start;

	block->request = http_request_new(task);

	block->task_next = task->blocks;
	task->blocks     = block;

	block->worker_next = NULL;

	return block;
}


void
dispatcher_block(struct task *task, struct block *block)
{
	downloader *dler = task->dler;
	struct worker *worker = dler->workers;

	/* TODO: 
	 * It just a temperary work around to select thread.
	 * Please change a way to load-balanced.
	 */
	int rad = rand() % 4;
	while (rad--) {
		worker = worker->next;
	}

	block->worker = worker;

	block->worker_next   = worker->waiting_list;
	worker->waiting_list = block;
	
	/* wake up the worker thread if sleeping */
	uv_async_send(worker->async);
}


void
get_block(struct block *block)
{
	struct task   *task   = block->task;
	struct worker *worker = block->worker;
	http_request  *req    = block->request;

	char addr[17] = { 0 };
	uv_ip4_name((struct sockaddr_in*)task->addrinfo->ai_addr, addr, 16);

	struct sockaddr_in dest;
	uv_ip4_addr(addr, http_url_get_port(task->url), &dest);

	uv_tcp_init(worker->loop, req->stream);

	req->stream->data  = block;
	uv_tcp_connect(req->connect, req->stream, (const struct sockaddr*)&dest, send_request);
}


static void
send_request(uv_connect_t *req, int status)
{
	if (status != 0) {
		printf("connect failed\n");
		exit(-1);
	}

	struct block *block = (struct block*)req->handle->data;
	struct task  *task = block->task;
	http_request *request = block->request;

	downloading = 1;
	uv_read_start(req->handle, on_alloc, on_read);

	//printf("tid: %d\n", block->worker->tid);
	request->stream = (uv_tcp_t*) req->handle;
	http_set_on_body(request, on_body);
	http_set_on_message_complete(request, on_message_complete);
	http_request_set_method(request, HTTP_GET);
	http_request_set_accept_range(request, block->start, block->end);
	http_send_request(request);
}


static void
on_alloc(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf)
{
	*buf = uv_buf_init(malloc(suggested_size), suggested_size);
}


static void
on_read(uv_stream_t* stream, ssize_t nread, const uv_buf_t *buf)
{
	ssize_t nparse;
	struct block *block = (struct block*)stream->data;
	http_request *req   = block->request;
	if (nread > 0) {
		nparse = http_parser_execute(req->http_parser, req->http_parser_setting, buf->base, nread);
		if (nparse < nread) {
			printf("parse incomplete: %ld/%ld parsed\n", nparse, nread);
		}
	} else if (nread < 0) {
		uv_read_stop(stream);
		uv_close((uv_handle_t*)stream, NULL);
		printf("nread < 0\n");
		system("pause");
		exit(-1);
	}

	free(buf->base);
}


static void
after_write(uv_write_t *req)
{
	free(req);
}


static int
on_body(http_parser *parser, const char *at, size_t length)
{
	http_request *request = (http_request*)parser->data;
	struct block *block   = (struct block*)request->stream->data;
	struct task  *task    = request->task;

	uv_fs_t *req = malloc(sizeof(uv_fs_t));
	uv_buf_t buf = uv_buf_init(at, length);

	/* FIXME:
	 * offset(7th argument) is the best block->downloaded_pos,
	 * but it make the file out of order while high pressure.
	 */
	/* uv_fs_write(block->worker->loop, req, task->fd, &buf, 1, -1, NULL); */
	uv_fs_write(block->worker->loop, req, task->fd, &buf, 1, block->downloaded_pos , NULL);

	block->downloaded_pos += length;

	return 0;
}


static int
on_message_complete(http_parser *parser)
{
	task **p, *task;
	http_request *req;
	downloader *dler;
	double speed;

	req  = parser->data;
	task = req->task;
	dler = task->dler;

	/* remove the task from tasks list */
	p = &dler->tasks;
	while (*p) {
		if (*p == task) {
			*p = task->next;
			speed = task->total_size * 1.0 / task->consumed_time;
			/* TODO: free the task resource */
			break;
		} else {
			p = &task->next;
		}
	}
	
	/* close the tcp connection */
	http_request_finish(req);

	
	printf("                                                                             \r");
	printf("download completed! average spped: %.2lf kb/s\n", speed);

	downloading = 0;

	return 0;
}
