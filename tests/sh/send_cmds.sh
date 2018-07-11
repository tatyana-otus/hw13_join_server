#!/bin/sh

../join_server 9001  &
P1=$!; sleep 1;
cat in.txt | nc -q 1 localhost 9001 > out.txt &
P2=$!;
wait $P2;
kill $P1