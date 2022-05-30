CFLAGS=-Wall -Wextra -Werror -pedantic -g
CC=clang $(CFLAGS)


httpserver:		httpserver.o requests.o response.o helper.o queue.o list.o pool.o node.o
				$(CC) -o httpserver httpserver.o requests.o response.o helper.o queue.o list.o pool.o node.o -pthread
httpserver.o:	requests.h response.h helper.h list.h httpserver.c
				$(CC) -c httpserver.c
dictionary.o:	dictionary.h list.h dictionary.c list.c
				$(CC) -c dictionary.c
queue.o:		queue.h list.h queue.c list.c
				$(CC) -c queue.c
list.o:			list.h requests.h response.h list.c requests.c response.c
				$(CC) -c list.c
requests.o:		requests.h helper.h node.h requests.c httpserver.c node.c
				$(CC) -c requests.c httpserver.c
response.o:		response.h requests.h helper.h node.h response.c httpserver.c node.c
				$(CC) -c response.c httpserver.c
helper.o:		helper.h helper.c
				$(CC) -c helper.c
pool.o:			pool.h pool.c
				$(CC) -c pool.c
node.o:			node.h node.c
				$(CC) -c node.c

clean:
				rm -f httpserver *.o
infer:
				make clean; infer-capture -- make; infer-analyze -- make
format:			
				clang-format -i -style=file *.[ch]
all:			httpserver