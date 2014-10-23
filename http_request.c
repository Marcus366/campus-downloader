#include <stdio.h>

#include "http_request.h"
#include "campus_downloader.h"

static void
after_fs_write(uv_fs_t *req)
{
	free(req);
}


http_request*
http_request_new(downloader *dler)
{
	http_request *req = malloc(sizeof(http_request));
	req->dler = dler;

	http_parser_init(&req->http_parser, HTTP_RESPONSE);
	req->http_parser.data = req;

	req->http_parser_setting = calloc(1, sizeof(http_parser_settings));

	return req;
}

static const uv_buf_t END_LINE = {.base = "\r\n", .len = 2};
static const uv_buf_t SPACE    = {.base = " ",    .len = 1};

void
http_send_request(http_request *req)
{
	uv_loop_t *loop = req->stream->loop;

	req->send_buf_cnt = 10;
	req->send_buf = malloc(req->send_buf_cnt * sizeof(uv_buf_t));

	uv_buf_t *buf = req->send_buf;
	buf[0] = uv_buf_init(http_method_str(HTTP_GET), strlen(http_method_str(HTTP_GET)));
	buf[1] = SPACE;
	buf[2] = http_url_get_field(req->dler->url, UF_PATH);
	buf[3] = SPACE;
	buf[4] = uv_buf_init("HTTP/1.1", sizeof("HTTP/1.1") - 1);
	buf[5] = END_LINE;
	buf[6] = uv_buf_init("Host: ", sizeof("Host: ") - 1);
	buf[7] = http_url_get_field(req->dler->url, UF_HOST);
	buf[8] = END_LINE;
	buf[9] = END_LINE;
	
	uv_write_t *write = malloc(sizeof(uv_write_t));
	uv_write(write, (uv_stream_t*)req->stream, req->send_buf, req->send_buf_cnt, NULL);
}