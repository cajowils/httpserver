#include <err.h>
#include <fcntl.h>
#include <inttypes.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/file.h>
#include <unistd.h>
#include <regex.h>
#include <pthread.h>

#include "requests.h"
#include "response.h"
#include "helper.h"
#include "list.h"
#include "queue.h"
#include "pool.h"

#define OPTIONS              "t:l:"
#define BUF_SIZE             4096
#define DEFAULT_THREAD_COUNT 4
#define MAX_QUEUE_SIZE 128

static FILE *logfile;
#define LOG(...) fprintf(logfile, __VA_ARGS__);
pthread_mutex_t log_lock = PTHREAD_MUTEX_INITIALIZER;

Pool p;

void send_response(struct response rsp, int connfd) {

    int line_size = (int) strlen(rsp.line.version) + (int) strlen(rsp.line.phrase)
                    + 8; //need 3 for the status code, 2 for spaces and 2 for the carriage return
    char *line_buf = (char *) calloc(1, sizeof(char) * line_size);
    snprintf(line_buf, line_size, "%s %d %s\r\n", rsp.line.version, rsp.line.code, rsp.line.phrase);
    write(connfd, line_buf, line_size - 1);
    free(line_buf);

    Node *ptr = rsp.headers->next;
    while (ptr != NULL) {
        int header_size = (int) strlen(ptr->head) + (int) strlen(ptr->val)
                          + 5; //1 for colon, 1 for space, 2 for \r\n
        char *header_buf = (char *) calloc(1, sizeof(char) * header_size);
        snprintf(header_buf, header_size, "%s: %s\r\n", ptr->head, ptr->val);
        write(connfd, header_buf, header_size - 1);
        ptr = ptr->next;
        free(header_buf);
    }
    char *creturn = "\r\n";
    write(connfd, creturn, strlen(creturn));

    if (rsp.mode == 0 && rsp.line.code == 200) { //checks that it is a successful GET request
        int size;
        int bytes = 2048;
        char buf2[bytes];
        while ((size = read(rsp.fd, buf2, bytes)) > 0) {
            write(connfd, buf2, size);
        }
    } else {

        int phrase_size = (int) strlen(rsp.line.phrase) + 2; //1 for /n
        char *phrase_buf = (char *) calloc(1, sizeof(char) * phrase_size);
        snprintf(phrase_buf, phrase_size, "%s\n", rsp.line.phrase);
        write(connfd, phrase_buf, phrase_size - 1);
        free(phrase_buf);
    }

    return;
}

void log_request(struct request req, struct response rsp) {
    //pthread_mutex_lock(&log_lock);
    if (req.line.method != NULL && req.line.URI != NULL) {
        LOG("%s,/%s,%d,%d\n", req.line.method, req.line.URI, rsp.line.code, req.ID);
        fflush(logfile);
    }
    //pthread_mutex_unlock(&log_lock);
}

// Creates a socket for listening for connections.
// Closes the program and prints an error message on error.
static int create_listen_socket(uint16_t port) {
    struct sockaddr_in addr;
    int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd < 0) {
        err(EXIT_FAILURE, "socket error");
    }
    memset(&addr, 0, sizeof addr);
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htons(INADDR_ANY);
    addr.sin_port = htons(port);
    if (bind(listenfd, (struct sockaddr *) &addr, sizeof addr) < 0) {
        err(EXIT_FAILURE, "bind error");
    }
    if (listen(listenfd, 128) < 0) {
        err(EXIT_FAILURE, "listen error");
    }
    return listenfd;
}

void handle_connection(int connfd) {
    // parse the buffer for all of the request information and put it in a request struct
    struct request req = parse_request_regex(connfd);

    //process the request and format it into a response
    struct response rsp = process_request(req);

    send_response(rsp, connfd);

    log_request(req, rsp);

    delete_request(req);
    delete_response(rsp);
    close(connfd);
    return;
}

static void sigterm_handler(int sig) {
    if (sig == SIGTERM) {
        warnx("received SIGTERM");
        destruct_pool(&p);
        fclose(logfile);
        exit(EXIT_SUCCESS);
    }
    if (sig == SIGINT) {
        warnx("received SIGINT");
        destruct_pool(&p);
        fclose(logfile);
        exit(EXIT_SUCCESS);
    }
}

static void usage(char *exec) {
    fprintf(stderr, "usage: %s [-t threads] [-l logfile] <port>\n", exec);
}

void queue_job(int connfd) {
    pthread_mutex_lock(&p.mutex);
    while (p.queue->size == p.queue->capacity) {
        pthread_cond_wait(&p.full, &p.mutex);
    }
    enqueue(p.queue, connfd);
    pthread_mutex_unlock(&p.mutex);
    pthread_cond_signal(&p.cond);
}

void *handle_thread() {
    while (p.running) {
        int connfd;
        pthread_mutex_lock(&p.mutex);
        //checks to see if there is anything in the queue
        if (peek(p.queue) == -1) {
            pthread_cond_wait(&p.cond, &p.mutex);
        }
        //grabs the first item in the queue
        connfd = dequeue(p.queue);
        pthread_cond_signal(&p.full);
        pthread_mutex_unlock(&p.mutex);
        if (connfd >= 0) {
            handle_connection(connfd);
        }
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    int opt = 0;
    int num_threads = DEFAULT_THREAD_COUNT;
    logfile = stderr;

    while ((opt = getopt(argc, argv, OPTIONS)) != -1) {
        switch (opt) {
        case 't':
            num_threads = strtol(optarg, NULL, 10);
            if (num_threads <= 0) {
                errx(EXIT_FAILURE, "bad number of threads");
            }
            break;
        case 'l':
            logfile = fopen(optarg, "w");
            if (!logfile) {
                errx(EXIT_FAILURE, "bad logfile");
            }
            break;
        default: usage(argv[0]); return EXIT_FAILURE;
        }
    }

    if (optind >= argc) {
        warnx("wrong number of arguments");
        usage(argv[0]);
        return EXIT_FAILURE;
    }

    uint16_t port = strtouint16(argv[optind]);
    if (port == 0) {
        errx(EXIT_FAILURE, "bad port number: %s", argv[1]);
    }

    signal(SIGPIPE, SIG_IGN);
    signal(SIGTERM, sigterm_handler);
    signal(SIGINT, sigterm_handler);

    int listenfd = create_listen_socket(port);
    //fcntl(listenfd, F_SETFL, O_NONBLOCK);

    initialze_pool(&p, num_threads, MAX_QUEUE_SIZE);

    for (int i = 0; i < num_threads; i++) {
        pthread_create(&p.threads[i], NULL, handle_thread, NULL);
    }

    for (;;) {
        int connfd = accept(listenfd, NULL, NULL);
        if (connfd < 0) {
            warn("accept error");
            continue;
        }
        if (connfd >= 0) {
            queue_job(connfd);
        }
    }

    return EXIT_SUCCESS;
}
