#!/bin/bash

arg1=${1:-1}
for ((i=1;i<=$arg1;i++))
do
    printf "INTERSECTION\nSYMMETRIC_DIFFERENCE\n" | nc localhost 9001 > /dev/null
done