# Assignment 0 - Split

This program takes in a single-character delimiter and an arbitrary amount of files in the format:
**./split <delimiter> <file1> <file2> ... <fileN>**

The objective is to produce an output which contains a newline in the place of each occurance of **<delimiter>** in each file. For example, upon reading **./split a foo.txt** where foo.txt contains the string: **bcdadeiabeidafaoeudh**, the program would produce:
```
bcd
dei
beid
f
oeudh
```

If a **'-'** is found in the place of a file, input is read in and processed from stdin. It is allowed, however, to have a mix between files and reads from stdin, in arbitrary order and quantity.

## Functionality

The main functionality of this program is opening files using open(), then reading them using read() and writing to stdout using write(). In main, the program iterates over all of the files given in argv, and decides how to process them. If it is a valid file, then it is passed to replace() with the appropriate fd. If it is **'-'**, then it is sent through replace() with an fd of 0, since that is the file descriptor of stdin. Otherwise, there is an error, which I will talk about below.

replace() is a function that takes in a file descriptor and a delimiter. It's purpose is to read() bytes into a buffer, iterate over the buffer to replace all instances of **<delimiter>** with **'\n'**, then write() the buffer to stdout. To optimize this process, I used a ternary statement rather than a conditional during the iteration, which assigns a value to buffer[i] depending on whether or not it is the delimiter. The reason I chose to make this function is because it saved me from reusing the same code for the stdin instances and file instances, rather, I was able to combine them by passing in their respective file descriptors.

## Errors

This program checks for errors in a variety of ways. While it is a small program, there are certainly still circumstances where it should be expected to fail.

The first instance of error checking occurs in main(), where the number of arguments (argc) is verified to be over 2. Otherwise, there are not a valid number of arguments, and errx() is used to display an error message and return with the error code of 22.

The next instance regards the length of the delimiter. If it is over 1 character long, then it is not valid and the same error handling is used as before.

In the case of a file that does not exist, or a file without permission to access, I check for this when using open(). If it returns a valid file, the program runs as expected, and otherwise uses warn() to display the error and later remembers to return that error number.

In the event that the output device is full when writing, the program uses errx() to show the issue and returns the error code, exiting the program.

## Reflection

I learned quite a bit in this assignment, including syscalls (open, read, write), the use of error handling (errx, warn), optimizing runtime in C, and running Linux. There were certain points were I had to make creative decisions that weren't necessarily outlined in the Lab Doc which I would like to talk about here.

I ended up using syscalls to read and write since we were not allowed to use certain functions in stdio. I found these functions to be very effective for the goal of the assignment, however, and am glad I have exposure to them now.

I ended up going with warn and errx because using printf and related functions sometimes called upon functions that were disallowed to us, but I found that these functions were equally as effective, and in certain cases, even more so, such as how warn() automatically displays the warning based off of the last error to occur.

I gave my buffer a size of 1000000 Bytes (1 MB), which might be overkill, but I found it to be very fast for large files during testing.

## Collaboration

In this assingment, I looked up the usage of functions such as read(), write(), open(), errx(), warn(), and found documentation on the man pages, as well as sites such as GeeksforGeeks and stackoverflow. All code is my own, but I thought I would reference these places as contributors.

I also discussed high level ideas on Discord in asgn0-general, and spoke with a tutor about optimization techniques.