#!/bin/sh

#rec=$(cat /dev/urandom | tr -dc 'a-zA-Z0-9' | fold -w 10 | head -n 1)
rec="aa_$1"
cmd="INSERT A $1 $rec"

#echo $cmd | nc -q 1 localhost 9001
echo $cmd | nc localhost 9001