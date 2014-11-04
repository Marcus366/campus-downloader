#ifndef __HTTP_REQUEST_H__
#define __HTTP_REQUEST_H__

#include <uv.h>

#include "third-party/http-parser/http_parser.h"

struct downloader;

typedef struct http_request {
	struct task          *task;
	uv_connect_t         *connect;
	uv_tcp_t             *stream;

	enum http_method      method;

	uint64_t              accept_range_start;
	uint64_t              accept_range_end;
	char                  accept_range_buf[32];
	
	uv_buf_t             *send_buf;
	size_t                send_buf_cnt;
	
	http_parser          *http_parser;
	http_parser_settings *http_parser_setting;


} http_request;


http_request *http_request_new(struct task *task);


void http_request_finish(struct http_request *req);

 
void http_request_set_accept_range(http_request *req, uint64_t start, uint64_t end);


void http_request_set_method(http_request *req, enum http_method method);


/* void http_request_set_url(http_request *req, const char *url);         */
/* void http_request_set_version(http_request *req, const char *version); */


void http_send_request(http_request *req);

/*
 * Interface for parsing callback.
 */
void http_set_on_message_begin(http_request *req, http_cb on_message_begin);
void http_set_on_url(http_request *req, http_data_cb on_url);
void http_set_on_status(http_request *req, http_data_cb on_status);
void http_set_on_header_field(http_request *req, http_data_cb on_header_field);
void http_set_on_header_value(http_request *req, http_data_cb on_header_value);
void http_set_on_headers_complete(http_request *req, http_cb on_headers_complete);
void http_set_on_body(http_request *req, http_data_cb on_body);
void http_set_on_message_complete(http_request *req, http_cb on_message_complete);

#endif
