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
#define REQUEST_LEN          4096
#define MAX_CONNECTIONS      128
#define READ                 0
#define WRITE                1

static FILE *logfile;
#define LOG(...) fprintf(logfile, __VA_ARGS__);
pthread_mutex_t log_lock = PTHREAD_MUTEX_INITIALIZER;

Pool p;

struct epoll_event events[MAX_CONNECTIONS];
int listenfd, nfds, epollfd;

int read_all(QueueNode *qn) {
    //non-blocking implementation of read. stores the request in the QueueNode until \r\n\r\n is seen, in which case it processes the request.
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
        bytes = read(qn->val, qn->buf + qn->size, REQUEST_LEN - qn->size);

        if (bytes > 0) {
            qn->size += bytes;
        }
        if (regexec(&re, qn->buf, 0, NULL, 0) == 0) {
            done = 1;
        }

    } while (bytes > 0 && qn->size < REQUEST_LEN && done != 1);
    regfree(&re);
    if (!done && bytes < 0 && errno == EWOULDBLOCK) {
        //blocked, returning later
        return -1;
    }
    return 1;
}

int write_all(QueueNode *qn) {
    //flushes the body that was read in with the request
    if (!qn->rsp.flushed) {
        write(qn->tmp, qn->buf + qn->req.body_start, qn->req.body_read);
        qn->rsp.flushed = 1;
    }

    int total_written = qn->req.body_read;

    int bytes = 4096;
    int size = 0;
    char *buf = (char *) calloc(BUF_SIZE, sizeof(char));
    int bytes_written = 0;

    int read_bytes = (qn->req.body_size - qn->req.body_read > bytes)
                         ? bytes
                         : qn->req.body_size - qn->req.body_read;

    //reads the bytes from the connfd and puts/appends them to a file
    //this implementation does not block, so it may come back to this function multiple times
    do {
        size = read(qn->val, buf, read_bytes);
        if (size < 0 && errno == EWOULDBLOCK) {
            //blocked, exiting here
            free(buf);
            return -1;
        }
        bytes_written = write(qn->tmp, buf, size);
        total_written += bytes_written;
        qn->req.body_read += size;
        read_bytes = (qn->req.body_size - qn->req.body_read > bytes)
                         ? bytes
                         : qn->req.body_size - qn->req.body_read;
    } while (size > 0 && qn->req.body_read < qn->req.body_size);
    //done reading the request
    free(buf);
    return 1;
}

void send_response(QueueNode *qn) {

    int line_size = (int) strlen(qn->rsp.line.version) + (int) strlen(qn->rsp.line.phrase)
                    + 8; //need 3 for the status code, 2 for spaces and 2 for the carriage return
    char *line_buf = (char *) malloc(sizeof(char) * line_size);
    memset(line_buf, '\0', line_size);
    snprintf(line_buf, line_size, "%s %d %s\r\n", qn->rsp.line.version, qn->rsp.line.code,
        qn->rsp.line.phrase);
    write(qn->val, line_buf, line_size - 1);
    free(line_buf);

    Node *ptr = qn->rsp.headers->next;
    while (ptr != NULL) {
        int header_size = (int) strlen(ptr->head) + (int) strlen(ptr->val)
                          + 5; //1 for colon, 1 for space, 2 for \r\n
        char *header_buf = (char *) malloc(sizeof(char) * header_size);
        memset(header_buf, '\0', header_size);
        snprintf(header_buf, header_size, "%s: %s\r\n", ptr->head, ptr->val);
        write(qn->val, header_buf, header_size - 1);
        ptr = ptr->next;
        free(header_buf);
    }
    char *creturn = "\r\n";
    write(qn->val, creturn, strlen(creturn));

    if (qn->rsp.mode == 0
        && qn->rsp.line.code == 200) { //checks that it is a successful GET request
        int size = 0;
        char *buf2 = (char *) calloc(BUF_SIZE, sizeof(char));
        memset(buf2, '\0', BUF_SIZE);

        while ((size = read(qn->tmp, buf2, BUF_SIZE)) > 0) {
            write(qn->val, buf2, size);
        }
    } else {

        int phrase_size = (int) strlen(qn->rsp.line.phrase) + 2; //1 for /n
        char *phrase_buf = (char *) malloc(sizeof(char) * phrase_size);
        memset(phrase_buf, '\0', phrase_size);
        snprintf(phrase_buf, phrase_size, "%s\n", qn->rsp.line.phrase);
        write(qn->val, phrase_buf, phrase_size - 1);
        free(phrase_buf);
    }

    return;
}

void log_request(QueueNode *qn) {
    pthread_mutex_lock(&log_lock);
    if (qn->req.line.method != NULL && qn->req.line.URI != NULL) {
        LOG("%s,/%s,%d,%d\n", qn->req.line.method, qn->req.line.URI, qn->rsp.line.code, qn->req.ID);
        fflush(logfile);
    }
    pthread_mutex_unlock(&log_lock);
}

void requeue_job(QueueNode *qn) {
    pthread_mutex_lock(&p.mutex);
    requeue(p.process_queue, qn);

    //adds the notification for the fd so that it can be alerted when new data is available
    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = qn->val;
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, qn->val, &ev) == -1) {
        warn("epoll_ctl: requeue connfd %d", qn->val);
    }
    pthread_mutex_unlock(&p.mutex);
    return;
}

void copy_file(int dest, int source) {
    lseek(source, 0, SEEK_SET);
    char *buf = (char *) calloc(BUF_SIZE, sizeof(char));
    memset(buf, '\0', BUF_SIZE);
    int size = 0;
    int bytes = 0;

    while ((size = read(source, buf, BUF_SIZE)) > 0) {
        bytes = write(dest, buf, size);
    };

    free(buf);
    return;
}

void handle_connection(QueueNode *qn) {
    if (qn->request != 1) {
        //checks that the request is fully read, otherwise proceeds to read it
        if (read_all(qn) == 1) {
            qn->request = 1;
            // parse the buffer for all of the request information and put it in a request struct

            qn->req = parse_request_regex(qn->buf, qn->size);

            //process the request and format it into a response
            qn->rsp = process_request(qn->req);

            //make temp file

            /*
            if mode == GET or APPEND {
                lock(file)
                flush file into temp
                unlock(file)
            }

            */
            if ((qn->tmp = mkstemp(qn->tmp_name)) == -1) {
                perror("error creating tmp");
            }
            unlink(qn->tmp_name);

            if (qn->rsp.mode == 0 || qn->rsp.mode == 2) {
                flock(qn->rsp.fd, LOCK_EX);
                copy_file(qn->tmp, qn->rsp.fd);
                flock(qn->rsp.fd, LOCK_UN);
            }

            switch (qn->rsp.mode) {
            case 0: {
                lseek(qn->tmp, 0, SEEK_SET);
                break;
            }
            case 1: {
                lseek(qn->tmp, 0, SEEK_SET);
                break;
            }
            case 2: {
                lseek(qn->tmp, 0, SEEK_CUR);
                break;
            }
            }

        } else {
            //if the read is blocked, add it to the process_queue to be processed later
            requeue_job(qn);
            return;
        }
    }

    //write_all to temp file

    //if the request is already processed, and it is a successful PUT/APPEND, continue to write the contents to the file
    if ((qn->req.mode == 1 || qn->req.mode == 2)
        && (qn->rsp.line.code == 200 || qn->rsp.line.code == 201)) {
        if (write_all(qn) != 1) {
            requeue_job(qn);
            return;
        }
    }

    send_response(qn);

    flock(qn->rsp.fd, LOCK_EX);
    if (qn->rsp.mode == 1) {
        ftruncate(qn->rsp.fd, 0);
    }
    copy_file(qn->rsp.fd, qn->tmp);
    log_request(qn);
    flock(qn->rsp.fd, LOCK_UN);

    //request is done, so destroy the queue node
    delete_queue_node(qn);

    return;
}

void *handle_thread() {
    //while the program is running
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
            //sends the request to be processed
            handle_connection(qn);
        }
    }
    return NULL;
}

void add_job(int connfd) {
    pthread_mutex_lock(&p.mutex);

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
    //joins the threads, frees the queues, and flushes the logfile once the program is killed
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

    //initializes the global threadpool struct
    initialze_pool(&p, num_threads, MAX_CONNECTIONS);

    //creates the threads
    for (int i = 0; i < num_threads; i++) {
        pthread_create(&p.threads[i], NULL, handle_thread, NULL);
    }

    //establishes epoll to listen for notifications on the fds that were interested in
    epollfd = epoll_create(MAX_CONNECTIONS);
    if (epollfd == -1) {
        perror("epoll_create");
        exit(EXIT_FAILURE);
    }

    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = listenfd;

    //adds the listenfd to the epoll
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, listenfd, &ev) == -1) {
        perror("epoll_ctl: listenfd");
        exit(EXIT_FAILURE);
    }

    for (;;) {
        //waits until an epoll notification is found
        nfds = epoll_wait(epollfd, events, MAX_CONNECTIONS, -1);
        if (nfds == -1) {
            warn("epoll wait failure");
            continue;
        }
        //iterates through the notifications it has received
        for (int n = 0; n < nfds; ++n) {
            //if the fd is listenfd, accept the new connection and add it to the epoll

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
                //otherwise, add the fd to the queue, as is has information to be processed.
                //remove the fd from the epoll so that multiple threads cannot work on the same connection

                ev.data.fd = events[n].data.fd;
                if (epoll_ctl(epollfd, EPOLL_CTL_DEL, events[n].data.fd, &ev) == -1) {
                    warn("epoll_ctl: processing connection %d", events[n].data.fd);
                } else if (events[n].data.fd > 0) {
                    add_job(events[n].data.fd);
                }
            }
        }
    }

    return EXIT_SUCCESS;
}
