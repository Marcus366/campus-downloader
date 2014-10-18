#include <stdio.h>
#include <uv.h>

#include "http_request.h"


void
on_alloc(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf)
{
	*buf = uv_buf_init(malloc(suggested_size), suggested_size);

}

void
on_read(uv_stream_t* stream, ssize_t nread, const uv_buf_t *buf)
{
	ssize_t nparse;
	http_request *req = (http_request*)stream->data;
	if (nread > 0) {
		nparse = http_parser_execute(&req->http_parser, req->http_parser_setting, buf->base, nread);
		if (nparse < nread) {
			printf("parse incomplete: %ld/%ld parsed\n", nparse, nread);
		}
	}

	free(buf->base);
}

void
on_write(uv_write_t *req, int status)
{
	printf("write done %d\n", status);
}

void
on_connect(uv_connect_t *req, int status)
{
	if (status == -1) {
		printf("on connect failed\n");
		return;
	}

	uv_read_start(req->handle, on_alloc, on_read);

	char out[] = "GET / HTTP/1.1\r\n"
		"Host: www.baidu.com\r\n"
         //"User-Agent: webclient.c\r\n"
         //"Keep-Alive: 100\r\n"
         //"Connection: keep-alive\r\n"
		 "\r\n";
	char *base = malloc(sizeof(out));
	memcpy(base, out, sizeof(out));
	uv_buf_t buf = uv_buf_init(base, sizeof(out));

	uv_write_t *write = malloc(sizeof(uv_write_t));
	uv_write(write, req->handle, &buf, 1, on_write);
}

void
on_resolved(uv_getaddrinfo_t *resolver, int status, struct addrinfo *res)
{
	uv_loop_t *loop = uv_default_loop();

	if (status == -1) {
		printf("Get addrinfo failed\n");
		return;
	}

	char addr[17] = { 0 };
	uv_ip4_name((struct sockaddr_in*)res->ai_addr, addr, 16);
	fprintf(stderr, "resolver ip %s\n", addr);

	struct sockaddr_in dest;
	uv_ip4_addr(addr, 80, &dest);

	uv_connect_t *connect = malloc(sizeof(uv_connect_t));
	uv_tcp_t *client = malloc(sizeof(uv_tcp_t));
	uv_tcp_init(loop, client);

	http_request *request = malloc(sizeof(http_request));
	http_request_init(request);
	request->stream = client;
	client->data = request;
	uv_tcp_connect(connect, client, (const struct sockaddr*)&dest, on_connect);

	uv_freeaddrinfo(res);
}

int
main(int argc, char** argv)
{
	uv_loop_t *loop = uv_default_loop();

	struct addrinfo hint;
	hint.ai_family = PF_INET;
	hint.ai_socktype = SOCK_STREAM;
	hint.ai_protocol = IPPROTO_TCP;
	hint.ai_flags = 0;

	uv_getaddrinfo_t resolver;
	uv_getaddrinfo(loop, &resolver, on_resolved, "www.baidu.com", NULL, &hint);

	uv_run(loop, UV_RUN_DEFAULT);

	system("pause");
	return 0;
}