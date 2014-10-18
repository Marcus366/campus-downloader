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