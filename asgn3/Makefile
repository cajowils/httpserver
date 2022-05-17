CFLAGS=-Wall -Wextra -Werror -pedantic -g
CC=clang $(CFLAGS)

httpserver:		httpserver.o requests.o response.o helper.o queue.o list.o pool.o
				$(CC) -o httpserver httpserver.o requests.o response.o helper.o queue.o list.o pool.o -lpthread
httpserver.o:	requests.h response.h helper.h list.h httpserver.c
				$(CC) -c httpserver.c
requests.o:		requests.h helper.h list.h requests.c httpserver.c
				$(CC) -c requests.c httpserver.c
response.o:		response.h requests.h helper.h list.h response.c httpserver.c
				$(CC) -c response.c httpserver.c
helper.o:		helper.h helper.c
				$(CC) -c helper.c
pool.o:			pool.h pool.c
				$(CC) -c pool.c
queue.o:		queue.h list.h queue.c list.c
				$(CC) -c queue.c
list.o:			list.h list.c
				$(CC) -c list.c
clean:
				rm -f httpserver *.o
infer:
				make clean; infer-capture -- make; infer-analyze -- make
format:			
				clang-format -i -style=file *.[ch]
all:			httpserver