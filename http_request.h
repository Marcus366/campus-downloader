#ifndef __HTTP_REQUEST_H__
#define __HTTP_REQUEST_H__

#include <uv.h>

#include "http_url.h"
#include "third-party/http-parser/http_parser.h"

typedef struct http_request {
	http_url             *url;
	uv_tcp_t             *stream;

	uv_buf_t             *send_buf;
	size_t                send_buf_cnt;
	
	http_parser           http_parser;
	http_parser_settings *http_parser_setting;



} http_request;

void http_request_init(http_request *req);


void http_request_set_method(http_request *req, const char *method);


void http_request_set_url(http_request *req, const char *url);


void http_request_set_version(http_request *req, const char *version);


void http_send_request(http_request *req);


#endif