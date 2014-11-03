CC=clang
CFLAGS=-g -Wall

OBJS=\
	main.o								\
	campus_downloader.o		\
	task.o								\
	worker.o							\
	block.o								\
	http_url.o						\
	http_request.o

downloader: $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) $<

main.o: main.c
	$(CC) $(CFLAGS) $^ -c $<

%.o: %.c %.h
	$(CC) $(CFLAGS) $^ -c $<
