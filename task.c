#include <stdio.h>
#include <fcntl.h>
#include "task.h"
#include "block.h"
#include "http_url.h"
#include "campus_downloader.h"
#include "third-party/http-parser/http_parser.h"


static void on_resolved(uv_getaddrinfo_t *req, int status, struct addrinfo *addrinfo);
static void send_head_request(uv_connect_t *req, int status);
static void on_alloc(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf);
static void on_head_read(uv_stream_t* stream, ssize_t nread, const uv_buf_t *buf);


static int on_header_field(http_parser *parser, const char *at, size_t length);
static int on_header_value(http_parser *parser, const char *at, size_t length);
static int on_headers_complete_close(http_parser *parser);


struct task*
create_task(downloader *dler, const char *url, const char *fullname)
{
	struct task *task = malloc(sizeof(struct task));
	
	task->dler          = dler;
	task->next          = dler->tasks;
	dler->tasks         = task;

	task->name          = _strdup(fullname);
	task->cur_size      = 0;

	task->blocks        = NULL;

	task->start_time    = uv_now(dler->mainloop);
	task->consumed_time = 0;

	task->last_step_time = task->start_time;
	task->last_step_size = task->cur_size;

	if (strncmp(url, "http://", 7) == 0) {
		task->url = http_parse_url(url);
	} else {
		int   len     = strlen(url);
		char *new_url = calloc(1, len + 7 + 1);
		memcpy(new_url, "http://", 7);
		memcpy(new_url + 7, url, len);

		task->url = http_parse_url(new_url);

		free(new_url);
	}

	if (task->url == NULL) {
		printf("parse url failed\n");
		exit(-1);
	}

	/* change host to a zero-terminated string for uv_getaddrinfo() */
	uv_buf_t buf = http_url_get_field(task->url, UF_HOST);
	char *host = calloc(1, buf.len + 1);
	memcpy(host, buf.base, buf.len);

	struct addrinfo hints;
	hints.ai_family   = PF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags    = 0;

	uv_getaddrinfo_t *getaddrinfo = malloc(sizeof(uv_getaddrinfo_t));
	getaddrinfo->data = task;

	uv_getaddrinfo(dler->mainloop, getaddrinfo, on_resolved, host, NULL, &hints);

	free(host);

	return task;
}


void
on_resolved(uv_getaddrinfo_t *req, int status, struct addrinfo *res)
{
	if (status != 0) {
		printf("on resolved failed: %s\n", uv_strerror(status));
		system("pause");
		exit(-1);
	}

	uv_loop_t  *loop = req->loop;
	task       *task = (struct task*)req->data;

	task->addrinfo = res;

	char addr[17] = { 0 };
	uv_ip4_name((struct sockaddr_in*)res->ai_addr, addr, 16);
	printf("resolver ip %s\n", addr);

	struct sockaddr_in dest;
	uv_ip4_addr(addr, http_url_get_port(task->url), &dest);

	uv_connect_t *connect = malloc(sizeof(uv_connect_t));
	uv_tcp_t *client = malloc(sizeof(uv_tcp_t));
	uv_tcp_init(loop, client);

	client->data = task;
	uv_tcp_connect(connect, client, (const struct sockaddr*)&dest, send_head_request);

	free(req);
}


void
send_head_request(uv_connect_t *req, int status)
{
	if (status != 0) {
		printf("on connect failed: %s\n", uv_strerror(status));
		return;
	}

	uv_read_start(req->handle, on_alloc, on_head_read);

	task *task = (struct task*)req->handle->data;

	task->head_request = http_request_new(task);
	http_request *request = task->head_request;
	request->stream = (uv_tcp_t*)req->handle;
	http_set_on_header_field(request, on_header_field);
	http_set_on_header_value(request, on_header_value);
	http_set_on_headers_complete(request, on_headers_complete_close);
	http_request_set_method(request, HTTP_HEAD);
	http_send_request(request);

	uv_fs_t fs;
	task->fd = uv_fs_open(req->handle->loop, &fs, task->name, O_CREAT | O_RDWR, 0777, NULL);
}


void
on_alloc(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf)
{
	*buf = uv_buf_init(malloc(suggested_size), suggested_size);
}


void
on_head_read(uv_stream_t* stream, ssize_t nread, const uv_buf_t *buf)
{
	ssize_t nparse;
	struct task *task = (struct task*)stream->data;
	http_request *req = task->head_request;
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


static int content_length = 0;

static int
on_header_field(http_parser *parser, const char *at, size_t length)
{
	char *str = malloc(length + 1);
	memcpy(str, at, length);
	str[length] = 0;
	printf("%s: ", str);

	if (strcmp(str, "Content-Length") == 0
		|| strcmp(str, "content-length") == 0) 
	{
		content_length = 1;
	} else {
		content_length = 0;
	}
	free(str);

	return 0;
}


static int
on_header_value(http_parser *parser, const char *at, size_t length)
{
	char *str = malloc(length + 1);
	memcpy(str, at, length);
	str[length] = 0;
	printf("%s\n", str);
	free(str);

	if (content_length) {
		http_request *request = (http_request*) parser->data;
		task         *task    = request->task;

		uint64_t *p = &task->total_size;
		int i;
		for (i = 0, *p = 0; length; ++i, --length) {
			*p = (*p) * 10 + at[i] - '0';
		}
	}

	return 0;
}


static int
on_headers_complete_close(http_parser *parser)
{
	http_request *req = (http_request*) parser->data;
	uv_loop_t   *loop = req->stream->loop;
	task        *task = (struct task*)req->task;

	http_request_finish(req);

	struct block *block = create_block(task, 0, task->total_size);

	dispatcher_block(task, block);

	return 0;
}