#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "http_request.h"
#include "campus_downloader.h"


http_request*
http_request_new(struct task *task)
{
	http_request *req = (http_request*)malloc(sizeof(struct http_request));

	req->connect = (uv_connect_t*)malloc(sizeof(uv_connect_t));
	req->stream  = (uv_tcp_t*)malloc(sizeof(uv_tcp_t));

	req->task = task;

	req->http_parser = (http_parser*)malloc(sizeof(http_parser));
	http_parser_init(req->http_parser, HTTP_RESPONSE);
	req->http_parser->data = req;

  req->use_accept_range    = 0;
	req->accept_range_start  = 0;
	req->accept_range_end    = 0;
	req->accept_range_buf[0] = '\0';

	/* req->http_parser_setting = calloc(1, sizeof(http_parser_settings)); */
	req->http_parser_setting = (http_parser_settings*)malloc(sizeof(http_parser_settings));
	req->http_parser_setting->on_body             = NULL;
	req->http_parser_setting->on_headers_complete = NULL;
	req->http_parser_setting->on_header_field     = NULL;
	req->http_parser_setting->on_header_value     = NULL;
	req->http_parser_setting->on_message_begin    = NULL;
	req->http_parser_setting->on_message_complete = NULL;
	req->http_parser_setting->on_status           = NULL;
	req->http_parser_setting->on_url              = NULL;

	return req;
}


void http_request_delete(http_request *request)
{
  free(request->connect);
  free(request->stream);

  free(request->http_parser);
  free(request->http_parser_setting);

  free(request);
}


void
http_request_close(http_request *req)
{
	uv_read_stop((uv_stream_t*)req->stream);
	uv_close((uv_handle_t*)req->stream, NULL);
}


void
http_request_set_accept_range(http_request *req, uint64_t start, uint64_t end)
{
  req->use_accept_range   = 1;
	req->accept_range_start = start;
	req->accept_range_end   = end;
	sprintf(req->accept_range_buf, "%lu-%lu", start, end);
}


static const uv_buf_t ENDLINE  = {.base = (char*)"\r\n", .len = 2};
static const uv_buf_t SPACE    = {.base = (char*)" ",    .len = 1};


void
http_send_request(http_request *req)
{
	enum state {
		REQ_METHOD,
		REQ_URL,
		REQ_VERSION,
		REQ_HOST,
		REQ_ACCEPT_RANGE,
		REQ_END,
	} req_state = REQ_METHOD;
	uint64_t i = 0;

	req->send_buf_cnt = 10;
	if (req->use_accept_range != 0) {
		req->send_buf_cnt += 3;
	}
	req->send_buf = (uv_buf_t*)malloc(req->send_buf_cnt * sizeof(uv_buf_t));

	uv_buf_t *buf = req->send_buf;
	while (i < req->send_buf_cnt) {
		switch (req_state) {
		case REQ_METHOD:
			buf[i++] = uv_buf_init((char*)http_method_str(req->method), strlen(http_method_str(req->method)));
			buf[i++] = SPACE;
			req_state = REQ_URL;
			break;
		case REQ_URL:
			buf[i++] = http_url_get_field(req->task->url, UF_PATH);
			buf[i++] = SPACE;
			req_state = REQ_VERSION;
			break;
		case REQ_VERSION:
			buf[i++] = uv_buf_init((char*)"HTTP/1.1", sizeof("HTTP/1.1") - 1);
			buf[i++] = ENDLINE;
			req_state = REQ_HOST;
			break;
		case REQ_HOST:
			buf[i++] = uv_buf_init((char*)"Host: ", sizeof("Host: ") - 1);
			buf[i++] = http_url_get_field(req->task->url, UF_HOST);
			buf[i++] = ENDLINE;
			req_state = REQ_ACCEPT_RANGE;
			break;
		case REQ_ACCEPT_RANGE:
			if (req->use_accept_range != 0) {
				buf[i++] = uv_buf_init((char*)"Range: bytes=", sizeof("Range: bytes=") - 1);
				buf[i++] = uv_buf_init(req->accept_range_buf, strlen(req->accept_range_buf));
				buf[i++] = ENDLINE;
			}
			req_state = REQ_END;
			break;
		case REQ_END:
			buf[i++] = ENDLINE;
			break;
		default:
			break;
		}
	}

	char str[1024];
	char *p = str;
	for (i = 0; i < req->send_buf_cnt; ++i) {
		memcpy(p, req->send_buf[i].base, req->send_buf[i].len);
		p += req->send_buf[i].len;
	}
	*p = 0;
	printf("%s", str);
	
	uv_write_t *write = (uv_write_t*)malloc(sizeof(uv_write_t));
	uv_write(write, (uv_stream_t*)req->stream, req->send_buf, req->send_buf_cnt, NULL);
}


void
http_request_set_method(http_request *req, enum http_method method)
{
	req->method = method;
}


void
http_set_on_message_begin(http_request *req, http_cb on_message_begin)
{
	req->http_parser_setting->on_message_begin = on_message_begin;
}


void
http_set_on_url(http_request *req, http_data_cb on_url)
{
	req->http_parser_setting->on_url = on_url;
}


void
http_set_on_status(http_request *req, http_data_cb on_status)
{
	req->http_parser_setting->on_status = on_status;
}


void
http_set_on_header_field(http_request *req, http_data_cb on_header_field)
{
	req->http_parser_setting->on_header_field = on_header_field;
}


void
http_set_on_header_value(http_request *req, http_data_cb on_header_value)
{
	req->http_parser_setting->on_header_value = on_header_value;
}


void
http_set_on_headers_complete(http_request *req, http_cb on_headers_complete)
{
	req->http_parser_setting->on_headers_complete = on_headers_complete;
}


void
http_set_on_body(http_request *req, http_data_cb on_body)
{
	req->http_parser_setting->on_body = on_body;
}


void
http_set_on_message_complete(http_request *req, http_cb on_message_complete)
{
	req->http_parser_setting->on_message_complete = on_message_complete;
}
