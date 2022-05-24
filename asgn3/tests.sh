#!/usr/bin/env bash

num_tests=5
port="$1"

#echo -e -n "GET /tests/test1.txt HTTP/1.1\r\n\r\n" | nc -N -C localhost $port | pv -L 5 > output.txt &
num=$((1))
for i in {1..100}
do
    echo -e -n "GET /tests/test$num.txt HTTP/1.1\r\n\r\n" | nc -N -C localhost $port &#> /dev/null &
    num=$((($i%$num_tests)+1))
done
