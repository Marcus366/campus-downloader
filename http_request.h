#ifndef __HTTP_REQUEST_H__
#define __HTTP_REQUEST_H__

#include <uv.h>

#include "third-party/http-parser/http_parser.h"

typedef struct http_request {
	uv_tcp_t             *stream;

	uv_buf_t              method;
	uv_buf_t			  url;
	uv_buf_t              version;

	uv_buf_t              send_buf;
	uv_buf_t              recv_buf;


	http_parser           http_parser;
	http_parser_settings *http_parser_setting;



} http_request;

void http_request_init(http_request *req);


void http_request_set_method(http_request *req, const char *method);


void http_request_set_url(http_request *req, const char *url);


void http_request_set_version(http_request *req, const char *version);

#endif