#!/bin/sh

rec="aa_$1"
cmd="INSERT A $1 $rec"
echo $cmd | nc localhost 9001