#!/usr/bin/env bash

num_tests=3
port="$1"


for i in {1..100}
do
    num=$((($i%$num_tests)+1))
    echo -e -n "GET /tests/test$num.txt HTTP/1.1\r\n\r\n" | nc -N -C localhost $port > /dev/null &
done
