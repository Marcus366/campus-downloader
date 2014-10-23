#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include "campus_downloader.h"
#include "third-party/http-parser/http_parser.h"


static void on_open(uv_fs_t *req);
static void on_resolved(uv_getaddrinfo_t *req, int status, struct addrinfo *addrinfo);
static void on_connect(uv_connect_t *req, int status);
static void on_alloc(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf);
static void on_read(uv_stream_t* stream, ssize_t nread, const uv_buf_t *buf);


static int on_header_field(http_parser *parser, const char *at, size_t length);
static int on_header_value(http_parser *parser, const char *at, size_t length);
static int on_body(http_parser *parser, const char *at, size_t length);
static int on_message_complete(http_parser *parser);


downloader*
downloader_new(const char *url, const char *fullname)
{
	downloader *dler = malloc(sizeof(downloader));

	if (strncmp(url, "http://", 7) == 0) {
		dler->url = http_parse_url(url);
	} else {
		int   len     = strlen(url);
		char *new_url = calloc(1, len + 7 + 1);
		memcpy(new_url, "http://", 7);
		memcpy(new_url + 7, url, len);

		dler->url = http_parse_url(new_url);

		free(new_url);
	}

	if (dler->url == NULL) {
		printf("parse url failed\n");
		exit(-1);
	}

	dler->request = http_request_new(dler);
	dler->task    = create_task(fullname);

	/* change host to a zero-terminated string for uv_getaddrinfo() */
	uv_buf_t buf = http_url_get_field(dler->url, UF_HOST);
	char *host = calloc(1, buf.len + 1);
	memcpy(host, buf.base, buf.len);

	struct addrinfo hints;
	hints.ai_family = PF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = 0;

	uv_getaddrinfo_t *getaddrinfo = malloc(sizeof(uv_getaddrinfo_t));
	getaddrinfo->data = dler;

	uv_getaddrinfo(uv_default_loop(), getaddrinfo, on_resolved, host, NULL, &hints);

	free(host);

	//uv_fs_open(uv_default_loop(), req, fullname, O_CREAT | O_RDWR, 0777, on_open);

	return dler;
}


int
downloader_run(downloader* dler)
{
	return uv_run(uv_default_loop(), UV_RUN_DEFAULT);
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
	downloader *dler = (downloader*)req->data;

	char addr[17] = { 0 };
	uv_ip4_name((struct sockaddr_in*)res->ai_addr, addr, 16);
	fprintf(stderr, "resolver ip %s\n", addr);

	struct sockaddr_in dest;
	uv_ip4_addr(addr, http_url_get_port(dler->url), &dest);

	uv_connect_t *connect = malloc(sizeof(uv_connect_t));
	uv_tcp_t *client = malloc(sizeof(uv_tcp_t));
	uv_tcp_init(loop, client);

	client->data = dler;
	uv_tcp_connect(connect, client, (const struct sockaddr*)&dest, on_connect);

	free(req);
}

void
on_connect(uv_connect_t *req, int status)
{
	if (status != 0) {
		printf("on connect failed: %s\n", uv_strerror(status));
		return;
	}

	uv_read_start(req->handle, on_alloc, on_read);

	downloader *dler = (downloader*) req->handle->data;

	http_request *request = dler->request;
	request->stream = (uv_tcp_t*)req->handle;
	http_set_on_header_field(request, on_header_field);
	http_set_on_header_value(request, on_header_value);
	http_set_on_body(request, on_body);
	http_set_on_message_complete(request, on_message_complete);
	http_send_request(request);

	uv_fs_t fs;
	dler->task->fd = uv_fs_open(uv_default_loop(), &fs, dler->task->name, O_CREAT | O_RDWR, 0777, NULL);
}

void
on_alloc(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf)
{
	*buf = uv_buf_init(malloc(suggested_size), suggested_size);
}

void
on_read(uv_stream_t* stream, ssize_t nread, const uv_buf_t *buf)
{
	ssize_t nparse;
	downloader *dler = (downloader*) stream->data;
	http_request *req = dler->request;
	if (nread > 0) {
		nparse = http_parser_execute(&req->http_parser, req->http_parser_setting, buf->base, nread);
		if (nparse < nread) {
			printf("parse incomplete: %ld/%ld parsed\n", nparse, nread);
		}
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
		downloader   *dler    = (downloader*) request->dler;
		task         *task    = dler->task;

		uint64_t *p = &task->total_size;
		*p = 0;
		while (length) {
			*p = (*p) * 10 + at[length - 1] - '0';
			--length;
		}
	}

	return 0;
}

static int
on_body(http_parser *parser, const char *at, size_t length)
{
	//char *str = malloc(length + 1);
	//memcpy(str, at, length);
	//str[length] = 0;
	//printf("\rbody: %s", str);
	//free(str);

	http_request *request = (http_request*) parser->data;
	downloader   *dler    = (downloader*) request->dler;
	task         *task    = dler->task;

	uv_fs_t *req = malloc(sizeof(uv_fs_t));
	uv_buf_t buf = uv_buf_init(at, length);
	uv_fs_write(uv_default_loop(), req, dler->task->fd, &buf, 1, -1, NULL);

	task->cur_size += length;
	printf("\rdownload %lld bytes /%lld bytes ============ %.2f%%", task->cur_size, task->total_size,
		   task->cur_size * 1.0f / task->total_size);

	return 0;
}

static int
on_message_complete(http_parser *parser)
{
	http_request *req  = parser->data;
	downloader   *dler = req->dler;

	uv_fs_t *fs = malloc(sizeof(uv_fs_t));

	uv_read_stop((uv_stream_t*)req->stream);
	uv_close((uv_handle_t*)req->stream, NULL);
	uv_fs_close(uv_default_loop(), fs, dler->task->fd, NULL);

	printf("\r                                                         ");
	printf("\rdownload completed!\n");

	return 0;
}
