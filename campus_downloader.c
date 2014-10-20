#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include "campus_downloader.h"
#include "third-party/http-parser/http_parser.h"


static void on_open(uv_fs_t *req);
static void on_resolved(uv_getaddrinfo_t *req, int status, struct addrinfo *addrinfo);
static void on_connect(uv_connect_t *req, int status);
static void on_alloc(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf);
static void on_read(uv_stream_t* stream, ssize_t nread, const uv_buf_t *buf);

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

	uv_fs_t *req = malloc(sizeof(uv_fs_t));
	req->data = dler;

	uv_fs_open(uv_default_loop(), req, fullname, O_CREAT | O_RDWR, 0777, on_open);

	return dler;
}


int
downloader_run(downloader* dler)
{
	return uv_run(uv_default_loop(), UV_RUN_DEFAULT);
}


void
on_open(uv_fs_t *req)
{
	if (req->result == -1) {
		printf("on open failed: %s\n", uv_strerror(req->result));
		system("pause");
		exit(-1);
	}
	
	downloader *dler = (downloader*)req->data;
	dler->file = req->result;

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
	uv_fs_req_cleanup(req);
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
	dler->request->stream = (uv_tcp_t*)req->handle;
	http_send_request(dler->request);
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