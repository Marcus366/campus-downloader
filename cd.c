#include <stdio.h>
#include <uv.h>

int
main(int argc, char** argv)
{
	uv_loop_t *loop = uv_loop_new();
	printf("hello libuv\n");
	uv_run(loop, UV_RUN_DEFAULT);
	return 0;
}