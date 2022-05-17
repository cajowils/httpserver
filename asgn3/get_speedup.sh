#!/usr/bin/env bash
# Author: Kabir Kwatra <kkwatra@ucsc.edu>
port=${1:-8080} # first arg if provided otherwise default (8080)
file=${2:-"README.md"}
logfile="kabtest.log"
threads=${3:-16}
connections=${4:-16}
requests=${5:-512}
seconds=${6:-5} # Timeout
logstdout=$logfile.stdout.log
logstderr=$logfile.stderr.log

./httpserver -t $threads -l $logfile $port > $logstdout 2> $logstderr &
pid=$!
while ! nc -zv localhost $port; do
    sleep 0.1
done

url="http://localhost:$port/$file"
urls=$(
    for _ in $(seq 1 $requests); do
        echo "-s -w %{http_code} -o /dev/null $url"
    done)
expected_statuses=$(for _ in $(seq 1 $requests); do echo -n "200"; done)

echo "Timing $requests Independent Requests"
start_time=$(date +%s%N)

statuses=$(
    timeout $seconds \
        curl --parallel --parallel-immediate \
        --parallel-max 1 \
        $urls)

end_time=$(date +%s%N)
itime=$((end_time-start_time))
if [[ "$statuses" != "$expected_statuses" ]]; then
    echo "Test Failed"
    if [[ "$statuses" == "" ]]; then
        echo "Probably Timedout (${seconds}s)"
    fi
    echo "Produced Statuses: $statuses"
    echo "Expected Statuses: $expected_statuses"
    kill -15 $pid # SIGTERM
    exit 1
fi

echo "Timing $requests Concurrent Requests"
start_time=$(date +%s%N)

statuses=$(
    timeout $seconds \
        curl --parallel --parallel-immediate \
        --parallel-max $connections \
        $urls)

end_time=$(date +%s%N)

ptime=$((end_time-start_time))

echo "Killing server $pid"
kill -15 $pid # SIGTERM
if [[ "$statuses" != "$expected_statuses" ]]; then
    echo "Test Failed"
    if [[ "$statuses" == "" ]]; then
        echo "Probably Timedout (${seconds}s)"
    fi
    echo "Produced Statuses: $statuses"
    echo "Expected Statuses: $expected_statuses"
    exit 1
fi
echo "Test Passed"
itimes=$((itime / 1000000000)).$((itime % 1000000000))
ptimes=$((ptime / 1000000000)).$((ptime % 1000000000))
echo "Independent Time: $itimes s"
echo "Parallel Time: $ptimes s"
echo "Approximate Speedup (with t=$threads): $((itime/ptime))x"
