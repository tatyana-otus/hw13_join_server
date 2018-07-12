#!/bin/bash

rm -f inters_sym_diff.txt expected_res.txt OK

../join_server 9001 &
sleep 1;

seq 1  5000 | xargs -n1 -P10 ../../tests/sh/gen_insert_A.sh > /dev/null & P1=$!;
seq 4000 -1 1001 | xargs -n1 -P10 ../../tests/sh/gen_insert_B.sh > /dev/null & P2=$!;
../../tests/sh/read_tables.sh 1000 & P3=$!;
wait $P1 $P2 $P3;

printf "INTERSECTION\nSYMMETRIC_DIFFERENCE\n" | nc -q 1 localhost 9001 > inters_sym_diff.txt

pkill join_server

for ((i=1001;i<=4000;i++))
do
echo $i,aa_$i,bb_$i>>expected_res.txt
done;
echo OK>>expected_res.txt
for ((i=1;i<=1000;i++))
do
echo $i,aa_$i,>>expected_res.txt
done;
for ((i=4001;i<=5000;i++))
do
echo $i,aa_$i,>>expected_res.txt
done;
echo OK>>expected_res.txt

f_diff=$(diff expected_res.txt inters_sym_diff.txt) 
if [ "$f_diff" = "" ] 
then
    touch OK
fi