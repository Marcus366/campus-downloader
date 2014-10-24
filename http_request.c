#include <stdio.h>

#include "http_request.h"
#include "campus_downloader.h"

static void
after_fs_write(uv_fs_t *req)
{
	free(req);
}


http_request*
http_request_new(struct task *task)
{
	http_request *req = malloc(sizeof(http_request));
	req->task = task;

	http_parser_init(&req->http_parser, HTTP_RESPONSE);
	req->http_parser.data = req;

	req->http_parser_setting = calloc(1, sizeof(http_parser_settings));

	return req;
}


void
http_request_finish(http_request *req)
{
	uv_read_stop((uv_stream_t*)req->stream);
	uv_close((uv_handle_t*)req->stream, NULL);

	free(req->http_parser_setting);
	free(req);
}


static const uv_buf_t ENDLINE  = {.base = "\r\n", .len = 2};
static const uv_buf_t SPACE    = {.base = " ",    .len = 1};


void
http_send_request(http_request *req)
{
	uv_loop_t *loop = req->stream->loop;

	req->send_buf_cnt = 10;
	req->send_buf = malloc(req->send_buf_cnt * sizeof(uv_buf_t));

	uv_buf_t *buf = req->send_buf;
	buf[0] = uv_buf_init(http_method_str(req->method), strlen(http_method_str(req->method)));
	buf[1] = SPACE;
	buf[2] = http_url_get_field(req->task->url, UF_PATH);
	buf[3] = SPACE;
	buf[4] = uv_buf_init("HTTP/1.1", sizeof("HTTP/1.1") - 1);
	buf[5] = ENDLINE;
	buf[6] = uv_buf_init("Host: ", sizeof("Host: ") - 1);
	buf[7] = http_url_get_field(req->task->url, UF_HOST);
	buf[8] = ENDLINE;
	buf[9] = ENDLINE;
	
	uv_write_t *write = malloc(sizeof(uv_write_t));
	uv_write(write, (uv_stream_t*)req->stream, req->send_buf, req->send_buf_cnt, NULL);
}