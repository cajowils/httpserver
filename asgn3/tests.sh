#!/usr/bin/env bash

file="tests/test3.txt"
port="$1"

#curl localhost:$port/$file & curl localhost:$port/$file & curl localhost:$port/$file & curl localhost:$port/$file & curl localhost:$port/$file & curl localhost:$port/$file & curl localhost:$port/$file & curl localhost:$port/$file & curl localhost:$port/$file & curl localhost:$port/$file & curl localhost:$port/$file & curl localhost:$port/$file

echo -e -n "GET /$file HTTP/1.1\r\n\r\n" | nc -N -C localhost $port & echo -e -n "GET /$file HTTP/1.1\r\n\r\n" | nc -N -C localhost $port & echo -e -n "GET /$file HTTP/1.1\r\n\r\n" | nc -N -C localhost $port & echo -e -n "GET /$file HTTP/1.1\r\n\r\n" | nc -N -C localhost $port & echo -e -n "GET /$file HTTP/1.1\r\n\r\n" | nc -N -C localhost $port & echo -e -n "GET /$file HTTP/1.1\r\n\r\n" | nc -N -C localhost $port & echo -e -n "GET /$file HTTP/1.1\r\n\r\n" | nc -N -C localhost $port & echo -e -n "GET /$file HTTP/1.1\r\n\r\n" | nc -N -C localhost $port & echo -e -n "GET /$file HTTP/1.1\r\n\r\n" | nc -N -C localhost $port & echo -e -n "GET /$file HTTP/1.1\r\n\r\n" | nc -N -C localhost $port