# Assignments 1,2 - HTTP Server

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

In Assignment 2, I made some very small changes to my Assignment 1 Code. I will detail these changes below.

## Getting Started

To run this program, you might find it helpful to use two terminal windows. In the first window, compile everything with 'make'. Then run './httpserver <port-number>' to open the communication link on any avaiable port of your choosing.
In the next window, you can test a request with Netcat. To do this, type in 

    echo -e -n "GET /foo.txt HTTP/1.1\r\n\r\n"
    | nc localhost <port-number> -l <logfile> -t <number of threads>
If you have foo.txt in the same directory as httpserver, then you should receive a 200 OK response, along with the contents of foo.txt

## Structures

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

## Functionality

### handle_connection()

This is the function that encompasses the steps I went over earlier. It calls the following functions in order to read, deserialize, process, serialize, and send requests.

### read_all()

This function reads into a buffer from the socket up to 2048 bytes, looking for a valid request indicator \r\n\r\n. Once the indicator has been found, or 2048 bytes have been read, it sends the total number of bytes that were read.

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

### finish_writing()

This writes all of the bytes within the Content-Length's specifications to a file from the socket. This allows the program to avoid using an unnecessarily large buffer to store the contents before hand. It reads and writes at most 4096 bytes at a time, and stops once the client halts the connection.

## Changes for Assignment 2
In Assignment 2, we have been tasked with taking in not only the port number, but also the number of threads and the file in which to store the log.
### Audit System
I implemented some new small features to accomodate a logging system. This allows the server to keep track of the requests that have been made and their statuses. The format of each line in the log looks as such:

        <Oper>,<URI>,<Status-Code>,<RequestID header value>\n
    
To do this, I used the predined LOG macro, which calls fprintf to the specified log file (defaults to stderr). The command line arguments are read in before the connection with the port is setup and the user has the option to run it with the -l flag, which is used to specify which file to flush the log to. 

As for Request-Id, I searched for the Request-Id header in my linked list and saved it as the requests Id number if it was found. Otherwise, it just defaults as 0.

If the request is valid, it will be added to the logfile.


## Reflection

This assignment was a doozy. I've spent more time on this than any other project I've had to work on in the past. That being said, I found it to be incredibly insightful and rewarding. I took Dr. Quinn's advice and approached this with a mix between "Top-Down" and "Bottom-Up" modularity, and found success in this. I was able to explore the requirements of the task and put together the modules as I went.

I started with parsing, and came up with a very broken solution after a little while of playing around with it. It read the request one character at a time and tried to format the request like that. It worked for a while, but struggled with invalid requests. I was able to get my other functions working pretty well with it, but once I finished them, I went back and redid the whole module using REGEX. This made my life so much easier, as it was able to check for invalidity very easily. Since I already had the other modules in place, it was incredibly easy to swap the old one for the new one.

I ended up learning to use Shell Script and came up with 53 tests validate my program. It runs each test against the output of the resource binary, and eventually I was able to use this to pass all 53. This was extremely nice for finding where bugs were coming from, and I will definitely use this strategy in the future.

## Collaboration
In this assignment, I referenced the man pages for a lot of functions, discussed high-level ideas from folks in the asgn1-general discord, and spoke to tutors and TAs about problems/ideas for my implementation. I also used Eugene's read_all() and part of write_all() that he gave us. That being said, all code is my own and I have not shown nor seen any code from classmates or any other resource that is prohibited.

I feel it is neccessary to mention that I referenced the Linked List code Eugene gave us back in Fall 2020 when I took CSE13s. While they are quite different now, I built the one in this project with that in mind.

When learning to match on REGEX expressions, I found a particular stack overflow post to be helpful:
https://stackoverflow.com/questions/2577193/how-do-you-capture-a-group-with-regex

I used some of the ideas by Ian Mackinnon in this post to help learn how to find matches, and give them credit for this.
