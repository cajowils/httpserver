#!/usr/bin/env bash

num_tests=3
port="$1"

#curl localhost:$port/$file & curl localhost:$port/$file & curl localhost:$port/$file & curl localhost:$port/$file & curl localhost:$port/$file & curl localhost:$port/$file & curl localhost:$port/$file & curl localhost:$port/$file & curl localhost:$port/$file & curl localhost:$port/$file & curl localhost:$port/$file & curl localhost:$port/$file

#echo -e -n "GET /$file HTTP/1.1\r\n\r\n" | nc -N -C localhost $port & echo -e -n "GET /$file HTTP/1.1\r\n\r\n" | nc -N -C localhost $port & echo -e -n "GET /$file HTTP/1.1\r\n\r\n" | nc -N -C localhost $port & echo -e -n "GET /$file HTTP/1.1\r\n\r\n" | nc -N -C localhost $port & echo -e -n "GET /$file HTTP/1.1\r\n\r\n" | nc -N -C localhost $port & echo -e -n "GET /$file HTTP/1.1\r\n\r\n" | nc -N -C localhost $port & echo -e -n "GET /$file HTTP/1.1\r\n\r\n" | nc -N -C localhost $port & echo -e -n "GET /$file HTTP/1.1\r\n\r\n" | nc -N -C localhost $port & echo -e -n "GET /$file HTTP/1.1\r\n\r\n" | nc -N -C localhost $port & echo -e -n "GET /$file HTTP/1.1\r\n\r\n" | nc -N -C localhost $port

for i in {1..1500}
do
    num=$((($i%$num_tests)+1))
    echo -e -n "GET /tests/test$num.txt HTTP/1.1\r\n\r\n" | nc -N -C localhost $port > /dev/null &
done
