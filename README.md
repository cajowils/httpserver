# Assignments 1,2,3,4 - HTTP Server

This program establishes a local connection through a port, from which HTTP requests/responses can be exchanged. It uses three methods to communicate with the client:
+ GET - The client requests the contents of a file from the server
+ PUT - The client wishes to create or update a file with contents in the request
+ APPEND - The client wishes to append a file with the contents in the request

Once a connection is made, the server goes through a process of steps to handle it:
1. Reading in the bytes from the client
2. Deserializing the information (validating and parsing the request)
3. Processing the request based on the method
4. Reading/Writing the specified files
5. Serializing and sending the response back to the client

In Assignment 1, I created the foundation for the server, including reading/parsing requests, formatting/sending responses, and handling files.

In Assignment 2, I made some very small changes to my Assignment 1 Code by adding an audit log and improving modularity.

In Assignment 3, I added multithreading and non-blocking IO to optimize the throughput of my program.

In Assignment 4, I implemented file locks to maintain atomicity in my server.

Below, I will share the details of my HTTP server and outline the steps in this iterative approach

## Getting Started

To run this program, you might find it helpful to use two terminal windows. In the first window, compile everything with 'make'. Then run './httpserver <port-number> -l <logfile> -t <num_threads>' to open the communication link on any avaiable port of your choosing.
In the next window, you can test a request with Netcat. To do this, type in 

    echo -e -n "GET /foo.txt HTTP/1.1\r\n\r\n"
    | nc localhost <port-number>
If you have foo.txt in the same directory as httpserver, then you should receive a 200 OK response, along with the contents of foo.txt

## Data Structures

In this program, I use a few data structures to help me keep track of my requests and responses.

### Request Struct

These store information to be used when parsing the request, such as:
* Method
* URI
* Version
* Body
* Headers (as a Linked List)

They also keeps track of how much of the body has been read, how many headers there are, the size of Content-Length, and if there is an error when parsing.

### Response Struct

The responses are similar to the requests. They store:
* Version
* Status Code
* Status Phrase
* Headers (as a Linked List)

They also keep track of the File Descriptor, the number of headers, and whether Content-Length has been added to the response.

### Linked List Struct

I used a linked list to store the headers of the requests and responses. This allows me to process an unspecified amount of headers, since we can't know beforehand how many we will need.
For "Content-Length: 100", each node consists of:
* head - the name of the header (i.e. Content-Length)
* val - the value of the header (i.e. 100)
* next - the next node in the linked list

### QueueNode Struct

I've made a second linked list node struct that contain information about each connection.
Here is some of the information that each node keeps track of:
* val - the fd of the connection
* buf - buffer to store the current request read in so far
* tmp - file descriptor for the temporary file created
* tmp_name - a string that stores the name of the temporary file
* req - the request struct
* rsp - the response struct
* next - pointer to the next node in the list/queue

### Queue Struct

I used a Queue to store connections as QueueNodes. The maximum length of a Queue should be 128, as that is the max number of connections that listenfd can keep track of. Here is what is stored in the Queue:
* head - the front of the queue, node first to be dequeued
* tail - the end of the queue, node most recently added
* size - the number of nodes in the queue

### Pool Struct

In order to keep track of and manage the threads, I've created a Thread Pool. This contains a lot of useful references, such as the threads themselves, the queues, the locks, and the conditional variables. Here is what it looks like:
* queue - the main worker queue where jobs are picked up from threads
* process_queue - the queue (more of a linked list) that blocking/unfinished jobs are added to in order to be proceessed later
* running - the state of the program; if running == 1, the threads will be active
* num_threads - the number of threads being used
* mutex - the lock used to ensual mutual exclusion between threads
* cond - the conditional variable used to alert threads when jobs become available

## Functionality

### handle_connection()

This is the function that encompasses the steps I went over earlier. It calls the following functions in order to read, deserialize, process, serialize, and send requests.

### read_all()

This function reads into a buffer from the socket up to 4096 bytes, looking for a valid request indicator \r\n\r\n. Once the indicator has been found, or 4096 bytes have been read, it sends the total number of bytes that were read.

### parse_request_regex()

To parse the request, I decided to use REGEX. I split the request into groupings to allow me to process each part of the request. Each group may capture the method, URI, version, or header field. If any of these parts are off, it sends 501 Not Implemented or 400 Bad Request as a response.

After checking the request line, I look through the header fields and store each one as a linked list node. If it finds Content-Length, it keeps a note of it's value for later use.

### process_request()

This function is mainly used as an abstraction for processing requests. It determines which method the request is asking for, and redirects it to one of the following functions.

### GET()

This handles GET requests. It attempts to open the file based on the URI provided, but may provide a variety of responses depending on the case.
* If the URI is a directory or has bad permissions, it will return 403: Forbidden
* If the URI doesn't exists, it will return 404: Not Found
* Otherwise, if it does exist and can be read from, it will return 200 OK

If the file is good, then it adds the Content-Length of it to the response struct and passes it along to status().

### PUT()

This handles PUT requests. Similar to GET(), it will have different results depending on the URI it is given.
* If the URI is a directory or has bad permissions, it will return 403: Forbidden
* If the URI doesn't exist, it will try to create the file
* Otherwise, it returns 200 OK and will write the contents of the body to the file

Upon success, it will dump the contents of the body into the file, then later write the rest of the bytes from the socket to the file.

### APPEND()

This handles APPEND requests. It is very similar to PUT(), with a few changes:
* If the file can't be found, it will return 404: Not Found

It will also write to the end of the file provided by the URI, rather than replacing it.

### status()

This function is used to finalize the response formatting. It takes in a response struct and an error code and sets the phrase and code appropriately. It also sets the Content-Length header for responses without a body.

### send_response()
Finishes formatting the response and sends the output to the socket. Upon a successful GET request, the body of the provided file will be written after the response line.

### write_all()

This writes all of the bytes within the Content-Length's specifications to a file from the socket. This allows the program to avoid using an unnecessarily large buffer to store the contents before hand. It reads and writes at most 4096 bytes at a time, and stops once the client halts the connection.

### handle_thread()

This is the body of each thread. When a thread is created, it is sent here and waits for work. Once work is available, it will dequeue the connection from the queue and send it to handle_connection() to be processed. Once thread_pool.running == 0, the thread will stop working and exit.

### add_job()

Takes a connfd and creates a new QueueNode to represent the connection. Enqueues the node to the work Queue.

### requeue_job()

Takes an existing connection node and adds it back to the work queue. Also adds notifications back to the fd so epoll will know once more data is available.

### find()

Searches through a linked-list (or linked-queue) for a specific connfd. If it is found, then that node is popped from the list and returned.

### enqueue()/requeue()/dequeue()

Standard queue operations. Difference between enqueue() and requeue() is that enqueue() takes a connfd and makes a new node to add to the queue, whereas requeue() takes an existing node and adds it to the queue.

### copy_file()

I made this function to copy the contents of one file to another. It uses a while loop to read the source file into a buffer and then writes the buffer into the destination file.


## Changes for Assignment 2
In Assignment 2, we have been tasked with taking in not only the port number, but also the number of threads and the file in which to store the log.
### Audit System
I implemented some new small features to accomodate a logging system. This allows the server to keep track of the requests that have been made and their statuses. The format of each line in the log looks as such:

        <Oper>,<URI>,<Status-Code>,<RequestID header value>\n
    
To do this, I used the predefined LOG macro, which calls fprintf to the specified log file (defaults to stderr). The command line arguments are read in before the connection with the port is setup and the user has the option to run it with the -l flag, which is used to specify which file to flush the log to. 

As for Request-Id, I searched for the Request-Id header in my linked list and saved it as the requests Id number if it was found. Otherwise, it just defaults as 0.

If the request is valid, it will be added to the logfile.

### New write_all()

I had some time to clean up some of the parts of asgn1 that I wasn't proud of, so I changed finish_writing() into write_all(). This allows me to abstract the process of reading bytes from connfd and writing to the request's URI. It returns the number of bytes written, and if there are any errors in reading/writing, it will response with a 500 code.

### Cleaner handle_connection()

I made some serious changes to how handle_connection() is modularized. I desinged it so that it only calls functions, and doesn't do any of the reading/writing itself. I put read_all() in my parsing function, and write_all() in the PUT and APPEND functions where it is more relevant. This allows me to narrow down bugs more easily and reduces complexity within the function itself.

## Changes for Assignment 3

In Assignment 3, we were tasked with implementing multiple threads into our HTTP server. To do this, I used the pthread library to create and manage my threads. We had to keep mutual exclusion in mind, and I used locks and conditional variables to do so. We also had to make our socket reads non-blocking.

### Multithreading

In order to implement multiple threads, I created a thread pool. This allowed me to keep track of all threads and their mutual exclusion elements, as well as the work queue and the list of unfinished requests. I used a global variable for the pool so that all functions would have access to the pool and its attributes.

### Dispatcher Thread/Non-Blocking IO

To implement a non-blocking IO, I used epoll. Using this, my dispatcher thread would be notified anytime there was new activity on the socket, whether it be a new connection or a current connection has sent more data. This change prevented a single request from taking a whole thread for an indefinite period of time, limiting the risk of denial-of-service attacks.

In order to add this functionality, I had to make some changes to my dispatcher thread. I used epoll_wait to wait until a notification arrived from any of the fds I was watching. Once a notification came in, I could add it to the queue to be picked up by a thread. If at any point the connection is to block during a read, the connection is added to the proceess_queue, where it will wait until epoll detects new data on that fd.

### Exiting

In order to clean everything up once the program ends, I had to destruct the thread pool in the signal handler. This signals all the threads to stop running with a broadcast signal, joins them, then frees them. It also frees the queues and destroys the locks and conditional variables. To make the threads exit successfully, I used a variable "running" in my thread handler to have the ability to stop the while() loop.

## Changes for Assignment 4

In assignment 4, our goal was to add atomicity to our server. This involved protecting reads and writes on the same file from overwriting one another. I had an idea involving a hashmap made up of queues originally, but I realized that it would have blocked every request after a write on the same URI until the write is finished, which is not a good idea for a server.

### File Locks

Instead, I chose to use flock() to provide some mutual exclusion when reading and writing to files. Whenever I want to read/write, I can flock() that file and copy the contents into a temporary file. This makes the server more efficient, as the critical region is smaller and other requests on that file can proceed without having to wait for one thread to finish. Otherwise, if I were to lock a file without using a temp, all other requests on the same URI would be blocked until that file is finished.

### Atomicity & Optimization

In order to efficiently implement atomicty, I wanted to have as small of a critical region as possible, as to avoid unnecessary waiting in the other threads. This is why the temp-file method is so effective for my implementation. It is able to caputure the file at one point in time, complete the modifications/readings, and then write the file all at once, after everything is completed. This avoids dependancy on things such as server speeds, connection latency, and eliminates a lot of data races from my program.

### The Specifics

Most of my changes were made in handle_connection. I ended up using a sneaky approach that minimzed changes to the system by tricking my program into thinking it was viewing or modifying the requested file, but it was only in control of the temporary file. I had to be more careful when opening files, as I learned when I discovered a bug that sometimes a file's content would be deleted if a PUT request to the same URI happened in the middle of the original one. I solved this by using opening the file normally with RDWR permissions, then calling ftruncate() once in the critical region to replace the contents of the file.

## Reflection

Assignment 1 was a doozy. I've spent more time on this than any other project I've had to work on in the past. That being said, I found it to be incredibly insightful and rewarding. I took Dr. Quinn's advice and approached this with a mix between "Top-Down" and "Bottom-Up" modularity, and found success in this. I was able to explore the requirements of the task and put together the modules as I went.

I started with parsing, and came up with a very broken solution after a little while of playing around with it. It read the request one character at a time and tried to format the request like that. It worked for a while, but struggled with invalid requests. I was able to get my other functions working pretty well with it, but once I finished them, I went back and redid the whole module using REGEX. This made my life so much easier, as it was able to check for invalidity very easily. Since I already had the other modules in place, it was incredibly easy to swap the old one for the new one.

I ended up learning to use Shell Script and came up with 53 tests validate my program. It runs each test against the output of the resource binary, and eventually I was able to use this to pass all 53. This was extremely nice for finding where bugs were coming from, and I will definitely use this strategy in the future.

In Assignment 2, I did some cleaning to allow for better modularity and clearer code. The core task didn't take too long to implement, as most of the work had already been done is Assignment 1. I went back and changed how I was reading to pass all of the tests in asgn1, and this helped my performance in asgn2 significantly.

In Asssignment 3, I reworked a lot of my previous code in order to implement non-blocking IO. I was able to handle multi-threading without changing too much, but once I had to pause a connection and store its information, thats's where things got tricky. The most challenging part was definitely getting everything to work together. There are a lot of functions being used and having multiple threads made it hard to keep track of where the program was at any given point. Debugging was a struggle because there are so many moving parts and a lot of non-determinism. What I mean by that is that sometimes bugs would appear in a scenario, but othertimes not. It made the process difficult because I couldn't always see the result of a bug, making it hard to track down. I found gdb to be helpful, as well as the toml files and test scripts that students posted in the Discord. All in all, I learned a lot in this assignment, and I have a much better understanding of how HTTP servers really work.

In Assignment 4, it took some careful strategizing before I wrote any code to be able to implement atomicity effectively. As i mentioned before, I had spent a few days designing a solution using dictionaries and queues, but I realized once I started testing that it was ultimately a bad idea due to the blocking consequences. This led me to do a complete U-Turn, and I had to spend some time thinking about how I could have a GET request safely execute in the middle of a write operation. I then started thinking about writing to temporary files, and using flock() was the obvious next step for adding file-specific mutual-exclusion.

Overall, the amount I learned in this assignment was greater than any other class I've ever had. I've spent countless hours coding this quarter, but I feel like the effort was worth it as I was able to combine a lot of the skills I already had and apply them in new ways. Looking back, I would have made things a lot cleaner, probably combining the request and response structs into one data structure, making the send_response() feel less cluttered, and using less files.

## Collaboration
In this assignment, I referenced the man pages for a lot of functions, discussed high-level ideas from folks in the Discord, and spoke to tutors and TAs about problems/ideas for my implementation. I also used Eugene's read_all() and part of write_all() that he gave us. That being said, all code is my own and I have not shown nor seen any code from classmates or any other resource that is prohibited.

I feel it is neccessary to mention that I referenced the Linked List code Eugene gave us back in Fall 2020 when I took CSE13s. While they are quite different now, I built the one in this project with that in mind.

I used scripts/toml files provided by other classmates when testing my program.

When learning to match on REGEX expressions, I found a particular stack overflow post to be helpful:
https://stackoverflow.com/questions/2577193/how-do-you-capture-a-group-with-regex

I used some of the ideas by Ian Mackinnon in this post to help learn how to find matches, and give them credit for this.

When learning about threads/epoll, I referenced the following resources:

https://www.youtube.com/watch?v=_n2hE2gyPxU
https://www.youtube.com/watch?v=P6Z5K8zmEmc&t=348
https://www.youtube.com/watch?v=WmDOHh7k0Ag
https://beej.us/guide/bgnet/html/#blocking
https://kovyrin.net/2006/04/13/epoll-asynchronous-network-programming/

I used the code in this post for fctrl, and Eugene gave it to us as well:
https://stackoverflow.com/questions/28897965/c-error-is-free-invalid-next-size-normal

I also used code straight from the epoll man page, and would like to credit the author.
