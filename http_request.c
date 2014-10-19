#include <stdio.h>

#include "http_request.h"

static int
on_header_field(http_parser *parser, const char *at, size_t length)
{
	char *str = malloc(length + 1);
	memcpy(str, at, length);
	str[length] = 0;
	printf("%s: ", str);
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

	return 0;
}

static int
on_body(http_parser *parser, const char *at, size_t length)
{
	char *str = malloc(length + 1);
	memcpy(str, at, length);
	str[length] = 0;
	printf("\nbody:\n%s\n", str);
	free(str);

	return 0;
}

static int
on_message_complete(http_parser *parser)
{
	http_request *req = parser->data;

	uv_read_stop((uv_stream_t*)req->stream);
	uv_close((uv_handle_t*)req->stream, NULL);

	printf("close tcp socket\n");

	return 0;
}

static http_parser_settings settings = { NULL, NULL, NULL, on_header_field, on_header_value, NULL, on_body, on_message_complete };


void
http_request_init(http_request *req)
{
	http_parser_init(&req->http_parser, HTTP_RESPONSE);
	req->http_parser.data = req;

	req->http_parser_setting = &settings;
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
	buf[2] = http_url_get_field(req->url, UF_PATH);
	buf[3] = SPACE;
	buf[4] = uv_buf_init("HTTP/1.1", sizeof("HTTP/1.1") - 1);
	buf[5] = END_LINE;
	buf[6] = uv_buf_init("Host: ", sizeof("Host: ") - 1);
	buf[7] = http_url_get_field(req->url, UF_HOST);
	buf[8] = END_LINE;
	buf[9] = END_LINE;
	
	uv_write_t *write = malloc(sizeof(uv_write_t));
	uv_write(write, req->stream, req->send_buf, req->send_buf_cnt, NULL);
}