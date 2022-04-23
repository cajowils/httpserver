CFLAGS=-Wall -Wextra -Werror -pedantic
CC=clang $(CFLAGS)

httpserver:		httpserver.o requests.o response.o helper.o
				$(CC) -o httpserver httpserver.o requests.o response.o helper.o
httpserver.o:	requests.h response.h helper.h httpserver.c
				$(CC) -c httpserver.c
requests.o:		requests.h requests.c
				$(CC) -c requests.c
response.o:		response.h requests.h helper.h response.c
				$(CC) -c response.c
helper.o:		helper.h helper.c
				$(CC) -c helper.c
clean:
				rm -f httpserver *.o
infer:
				make clean; infer-capture -- make; infer-analyze -- make
format:
				clang-format *.c *.h
all:			httpserver