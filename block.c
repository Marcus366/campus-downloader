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
	block->pos   = start;
	block->end   = end;

	block->request = http_request_new(task);

	return block;
}


void
get_block(struct block *block)
{
	struct task *task = block->task;

	char addr[17] = { 0 };
	uv_ip4_name((struct sockaddr_in*)task->addrinfo->ai_addr, addr, 16);

	struct sockaddr_in dest;
	uv_ip4_addr(addr, http_url_get_port(task->url), &dest);

	uv_connect_t *connect = malloc(sizeof(uv_connect_t));
	uv_tcp_t *client = malloc(sizeof(uv_tcp_t));
	uv_tcp_init(uv_default_loop(), client);

	connect->data = block;
	client->data  = block;
	uv_tcp_connect(connect, client, (const struct sockaddr*)&dest, send_request);
}


static void
send_request(uv_connect_t *req, int status)
{
	if (status != 0) {
		printf("connect failed\n");
		exit(-1);
	}

	struct block *block = (struct block*)req->data;
	struct task  *task = block->task;
	http_request *request = block->request;

	downloading = 1;
	uv_read_start(req->handle, on_alloc, on_read);

	request->stream = (uv_tcp_t*) req->handle;
	http_set_on_body(request, on_body);
	http_set_on_message_complete(request, on_message_complete);
	http_request_set_method(request, HTTP_GET);
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
		nparse = http_parser_execute(&req->http_parser, req->http_parser_setting, buf->base, nread);
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


static int
on_body(http_parser *parser, const char *at, size_t length)
{
	http_request *request = (http_request*) parser->data;
	task         *task    = request->task;

	uv_fs_t *req = malloc(sizeof(uv_fs_t));
	uv_buf_t buf = uv_buf_init(at, length);
	uv_fs_write(uv_default_loop(), req, task->fd, &buf, 1, -1, NULL);

	task->cur_size += length;

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
	p = &dler->task;
	while (*p) {
		if (*p == task) {
			*p = task->next;
			speed = task->total_size * 1.0 / task->consumed_time;
			/* TODO free the task resource */
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
