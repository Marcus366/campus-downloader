CC=clang
CFLAGS=-g -Wall

OBJS=\
	main.o								\
	campus_downloader.o		\
	task.o								\
	worker.o							\
	block.o								\
	http_request.o				\
	http_url.o						\
	http_parser.o

downloader: $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -luv -o $@

main.o: main.c
	$(CC) $(CFLAGS) $^ -c $<

%.o: %.c %.h
	$(CC) $(CFLAGS) $^ -c $<
