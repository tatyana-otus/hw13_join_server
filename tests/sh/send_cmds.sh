#!/bin/sh

../join_server 45232  &
P1=$!; sleep 1;
cat in.txt | nc -q 1 localhost 45232 > out.txt &
P2=$!;
wait $P2;
kill $P1