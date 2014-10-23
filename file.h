#ifndef __FILE_H__
#define __FILE_H__

#include <uv.h>

typedef struct file {
	uv_file fd;

	uint64_t offset;

} file;

#endif