#include <stdlib.h>
#include <string.h>

#include "http_url.h"


http_url*
http_parse_url(const char *str)
{
	http_url *url = malloc(sizeof(http_url));

	int   len  = strlen(str);
	char *base = malloc(len);
	memcpy(base, str, len);

	url->raw_url = uv_buf_init(base, len);

	if (http_parser_parse_url(base, len, 0, &url->result) != 0) {
		printf("parse url failed\n");
		goto fail;
	}

	return url;

fail:
	http_free_url(url);
	return NULL;
}

void
http_free_url(http_url *url)
{
	free(url->raw_url.base);
	free(url);
}

uv_buf_t
http_url_get_field(http_url *url, http_parser_url_fields field)
{
	http_parser_url *parser = &url->result;

	if ((1 << field) & parser->field_set) {
		return uv_buf_init(url->raw_url.base + parser->field_data[field].off, parser->field_data[field].len);
	} else {
		if (field == UF_PATH) {
			return uv_buf_init("/", 1);
		}
		return uv_buf_init(NULL, 0);
	}
}