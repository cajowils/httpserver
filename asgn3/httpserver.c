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
#include <sys/epoll.h>
#include <errno.h>

#include "requests.h"
#include "response.h"
#include "helper.h"
#include "list.h"
#include "queue.h"
#include "pool.h"

#define OPTIONS              "t:l:"
#define BUF_SIZE             4096
#define DEFAULT_THREAD_COUNT 4
#define REQUEST_LEN          2048
#define MAX_CONNECTIONS      128

static FILE *logfile;
#define LOG(...) fprintf(logfile, __VA_ARGS__);
pthread_mutex_t log_lock = PTHREAD_MUTEX_INITIALIZER;

Pool p;

struct epoll_event events[MAX_CONNECTIONS];
int listenfd, nfds, epollfd;

int read_all(QueueNode *qn) {
    int bytes = 0;
    int done = 0;
    const char *pattern = "\r\n\r\n";
    regex_t re;
    if (regcomp(&re, pattern, REG_EXTENDED) != 0) {
        regfree(&re);
        return -1;
    }
    do {
        errno = 0;
        bytes = read(qn->val, qn->buf + qn->size, 1);

        if (bytes > 0) {
            qn->size += bytes;
        }
        if (regexec(&re, qn->buf, 0, NULL, 0) == 0) {
            done = 1;
        }

    } while (bytes > 0 && qn->size < REQUEST_LEN && done != 1);
    regfree(&re);
    if (!done && bytes < 0 && errno == EWOULDBLOCK) {
        //printf("Blocked!\nfd: %d\nbuf: %s\n",qn->val,qn->buf);
        return -1;
    }
    //printf("Passed!\nfd: %d\nbuf: %s\n",qn->val,qn->buf);
    return 1;
}

int write_all(QueueNode *qn) {
    int total_written = 0;
    //flushes the body that was read in with the request
    /*if ((total_written = write(rsp.fd, req.body, req.body_read)) < 0) {
        return -1;
    }*/

    int bytes = 4096;
    int size = 0;
    char *buf = (char *) calloc(1, sizeof(char) * bytes);
    int bytes_written = 0;

    int read_bytes
        = (qn->body_size - qn->body_read > bytes) ? bytes : qn->body_size - qn->body_read;
    do {
        size = read(qn->val, buf, read_bytes);
        if (size < 0 && errno == EWOULDBLOCK) {
            free(buf);
            return -1;
        }
        bytes_written = write(qn->req_fd, buf, size);
        total_written += bytes_written;
        qn->body_read += size;
        read_bytes
            = (qn->body_size - qn->body_read > bytes) ? bytes : qn->body_size - qn->body_read;
    } while (size > 0 && qn->body_read < qn->body_size);
    free(buf);

    return 1;
}

void send_response(struct response rsp, int connfd) {

    int line_size = (int) strlen(rsp.line.version) + (int) strlen(rsp.line.phrase)
                    + 8; //need 3 for the status code, 2 for spaces and 2 for the carriage return
    char *line_buf = (char *) malloc(sizeof(char) * line_size);
    memset(line_buf, '\0', line_size);
    snprintf(line_buf, line_size, "%s %d %s\r\n", rsp.line.version, rsp.line.code, rsp.line.phrase);
    write(connfd, line_buf, line_size - 1);
    free(line_buf);

    Node *ptr = rsp.headers->next;
    while (ptr != NULL) {
        int header_size = (int) strlen(ptr->head) + (int) strlen(ptr->val)
                          + 5; //1 for colon, 1 for space, 2 for \r\n
        char *header_buf = (char *) malloc(sizeof(char) * header_size);
        memset(header_buf, '\0', header_size);
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
        char *phrase_buf = (char *) malloc(sizeof(char) * phrase_size);
        memset(phrase_buf, '\0', phrase_size);
        snprintf(phrase_buf, phrase_size, "%s\n", rsp.line.phrase);
        write(connfd, phrase_buf, phrase_size - 1);
        free(phrase_buf);
    }

    return;
}

void log_request(struct request req, struct response rsp) {
    pthread_mutex_lock(&log_lock);
    if (req.line.method != NULL && req.line.URI != NULL) {
        LOG("%s,/%s,%d,%d\n", req.line.method, req.line.URI, rsp.line.code, req.ID);
        fflush(logfile);
    }
    pthread_mutex_unlock(&log_lock);
}

void requeue_job(QueueNode *qn) {
    pthread_mutex_lock(&p.mutex);
    while (queues_full(&p)) {
        pthread_cond_wait(&p.full, &p.mutex);
    }
    requeue(p.process_queue, qn);
    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = qn->val;
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, qn->val, &ev) == -1) {
        warn("epoll_ctl: requeue connfd %d", qn->val);
    }
    //printf("requeing fd: %d\n", qn->val);
    pthread_cond_signal(&p.full);
    pthread_mutex_unlock(&p.mutex);
    return;

    //adds the notification for the fd again so that it can be read from
}

void handle_connection(QueueNode *qn) {
    if (qn->request != 1) {
        if (read_all(qn) == 1) {
            qn->request = 1;
            // parse the buffer for all of the request information and put it in a request struct
            //printf("request: %s\nsize: %d\n", qn->buf, qn->size);
            struct request req = parse_request_regex(qn->buf, qn->size);
            req.connfd = qn->val;

            //process the request and format it into a response
            struct response rsp = process_request(req);

            send_response(rsp, qn->val);

            log_request(req, rsp);

            qn->body_size = req.body_size;
            qn->body_read = req.body_read;
            qn->mode = rsp.mode;
            qn->req_fd = rsp.fd;
            qn->code = rsp.line.code;

            delete_request(req);
            delete_response(rsp);
        } else {
            requeue_job(qn);
            return;
        }
    }
    if (qn->request == 1) {
        if ((qn->mode == 1 || qn->mode == 2) && (qn->code == 200 || qn->code == 201)) {
            if (write_all(qn) != 1) {
                requeue_job(qn);
                return;
            }
        }

        delete_queue_node(qn);
    }

    return;
}

void *handle_thread() {
    while (p.running) {
        QueueNode *qn = NULL;
        pthread_mutex_lock(&p.mutex);
        //checks to see if there is anything in the queue
        if (empty(p.queue)) {
            pthread_cond_wait(&p.cond, &p.mutex);
        }
        //grabs the first item in the queue
        qn = dequeue(p.queue);
        pthread_cond_signal(&p.full);
        pthread_mutex_unlock(&p.mutex);
        if (qn && qn->val >= 0) {
            handle_connection(qn);
        }
    }
    return NULL;
}

void add_job(int connfd) {
    //printf("New Node:\nReq: %s\nSize: %d\nfd: %d\n", tmp->buf, tmp->size, tmp->val);
    pthread_mutex_lock(&p.mutex);
    while (queues_full(&p)) {
        pthread_cond_wait(&p.full, &p.mutex);
    }
    //search linked list for fd, if it is found then enqueue it, otherwise create one and enqueue it
    QueueNode *qn;
    if (p.queue && p.process_queue) {
        if ((qn = find(p.process_queue, connfd)) != NULL) {
            requeue(p.queue, qn);
        } else {
            enqueue(p.queue, connfd);
        }
    }
    pthread_cond_signal(&p.cond);
    pthread_mutex_unlock(&p.mutex);
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
    if (listen(listenfd, MAX_CONNECTIONS) < 0) {
        err(EXIT_FAILURE, "listen error");
    }
    return listenfd;
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
    /*int flags = fcntl(listenfd, F_GETFL, 0);
    fcntl(listenfd, F_SETFL, flags | O_NONBLOCK);*/

    initialze_pool(&p, num_threads, MAX_CONNECTIONS);

    for (int i = 0; i < num_threads; i++) {
        pthread_create(&p.threads[i], NULL, handle_thread, NULL);
    }

    epollfd = epoll_create(MAX_CONNECTIONS);
    if (epollfd == -1) {
        perror("epoll_create");
        exit(EXIT_FAILURE);
    }
    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = listenfd;
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, listenfd, &ev) == -1) {
        perror("epoll_ctl: listenfd");
        exit(EXIT_FAILURE);
    }

    for (;;) {
        nfds = epoll_wait(epollfd, events, MAX_CONNECTIONS, -1);
        if (nfds == -1) {
            warn("epoll wait failure");
            continue;
        }

        for (int n = 0; n < nfds; ++n) {
            if (events[n].data.fd == listenfd) {
                int connfd = accept(listenfd, NULL, NULL);
                if (connfd < 0) {
                    warn("accept error");
                    continue;
                }

                int flags = fcntl(connfd, F_GETFL, 0);
                fcntl(connfd, F_SETFL, flags | O_NONBLOCK);

                ev.events = EPOLLIN;
                ev.data.fd = connfd;
                if (epoll_ctl(epollfd, EPOLL_CTL_ADD, connfd, &ev) == -1) {
                    warn("epoll_ctl: new connection %d", connfd);
                    continue;
                }
            } else {
                /*printf("fd: %d\n", events[n].data.fd);
                print_queue(p.queue);
                printf("Process Queue:\n");
                print_queue(p.process_queue);*/
                ev.data.fd = events[n].data.fd;
                if (epoll_ctl(epollfd, EPOLL_CTL_DEL, events[n].data.fd, &ev) == -1) {
                    warn("epoll_ctl: processing connection %d", events[n].data.fd);
                    return EXIT_FAILURE;
                } else if (events[n].data.fd > 0) {
                    add_job(events[n].data.fd);
                }
            }
        }
    }

    return EXIT_SUCCESS;
}
