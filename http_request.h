#ifndef __HTTP_REQUEST_H__
#define __HTTP_REQUEST_H__

#include <uv.h>

#include "third-party/http-parser/http_parser.h"

extern struct downloader;

typedef struct http_request {
	struct downloader	 *dler;
	uv_tcp_t             *stream;

	uv_buf_t             *send_buf;
	size_t                send_buf_cnt;
	
	http_parser           http_parser;
	http_parser_settings *http_parser_setting;

} http_request;


http_request *http_request_new(struct downloader *dler);


void http_request_set_method(http_request *req, const char *method);


void http_request_set_url(http_request *req, const char *url);


void http_request_set_version(http_request *req, const char *version);


void http_send_request(http_request *req);

static void
http_set_on_message_begin(http_request *req, http_cb on_message_begin)
{
	req->http_parser_setting->on_message_begin = on_message_begin;
}

static void
http_set_on_url(http_request *req, http_data_cb on_url)
{
	req->http_parser_setting->on_url = on_url;
}

static void
http_set_on_status(http_request *req, http_data_cb on_status)
{
	req->http_parser_setting->on_status = on_status;
}

static void
http_set_on_header_field(http_request *req, http_data_cb on_header_field)
{
	req->http_parser_setting->on_header_field = on_header_field;
}

static void
http_set_on_header_value(http_request *req, http_data_cb on_header_value)
{
	req->http_parser_setting->on_header_value = on_header_value;
}

static void
http_set_on_headers_complete(http_request *req, http_cb on_headers_complete)
{
	req->http_parser_setting->on_headers_complete = on_headers_complete;
}

static void
http_set_on_body(http_request *req, http_data_cb on_body)
{
	req->http_parser_setting->on_body = on_body;
}

static void
http_set_on_message_complete(http_request *req, http_cb on_message_complete)
{
	req->http_parser_setting->on_message_complete = on_message_complete;
}

#endif