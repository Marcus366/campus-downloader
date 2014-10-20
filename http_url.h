#ifndef __HTTP_URL_H__
#define __HTTP_URL_H__

#include <uv.h>

#include "third-party/http-parser/http_parser.h"


typedef struct http_parser_url        http_parser_url;
typedef enum   http_parser_url_fields http_parser_url_fields;


typedef struct http_url {
	uv_buf_t         raw_url;

	http_parser_url  result;
} http_url;


http_url* http_parse_url(const char *str);
void      http_free_url(http_url *url);

uv_buf_t  http_url_get_field(http_url *url, http_parser_url_fields field);

int       http_url_get_port(http_url *url);

#endif