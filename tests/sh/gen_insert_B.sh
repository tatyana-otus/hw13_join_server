#!/bin/sh

rec="bb_$1"
cmd="INSERT B $1 $rec"
echo $cmd | nc localhost 9001