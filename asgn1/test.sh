#!/usr/bin/env bash
<<com
for i in {1..1}; do
    printf "GET /tests/foo5.txt HTTP/1.1\r\n\r\n" > request
    
    cat request | nc -N localhost 8080 > response

    head -n 1 response

    expected_code=200
    status_code=$(head -n 1 response | awk '{ print $2 }')
    if [[ status_code -eq expected_code ]]; then
        echo "Test Passed"
    else
        echo "Test Failed"
    fi

done
com

input="tests.txt"
regex="\"(.*)\"(\s[^""\s]*)*"
while IFD= read -r line
do
    val="${line:0:4}"
    if [[ $val = "TEST" ]]; then

        if [[ $line =~ $regex ]]
        then
            #test_no="${BASH_REMATCH[1]}"
            request="${BASH_REMATCH[1]}"
            #echo "$request"
            printf "$request" > req
            cat req | nc -N localhost 8080 > response
            expected_code=200
            status_code=$(head -n 1 response | awk '{ print $2 }')
            if [[ status_code -eq expected_code ]]; then
                echo "Test $test_no Passed"
            else
                echo "Test $test_no Failed"
            fi

        else
            echo "No Match on Test ${line:4:1}"
        fi
        #echo "$line"
        #ut -d '"' -f2 < "$line"
        #request=$(echo "$line" | awk '{print $2}')
        #echo "$request"
    fi
done < "$input"


