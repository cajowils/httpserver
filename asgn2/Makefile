CFLAGS=-Wall -Wextra -Werror -pedantic -g
CC=clang $(CFLAGS)

httpserver:		httpserver.o requests.o response.o helper.o list.o
				$(CC) -o httpserver httpserver.o requests.o response.o helper.o list.o
httpserver.o:	requests.h response.h helper.h list.h httpserver.c
				$(CC) -c httpserver.c
requests.o:		requests.h helper.h list.h requests.c
				$(CC) -c requests.c
response.o:		response.h requests.h helper.h list.h response.c
				$(CC) -c response.c
helper.o:		helper.h helper.c
				$(CC) -c helper.c
list.o:			list.h list.c
				$(CC) -c list.c
clean:
				rm -f httpserver *.o
infer:
				make clean; infer-capture -- make; infer-analyze -- make
format:			
				clang-format -i -style=file *.[ch]
all:			httpserver