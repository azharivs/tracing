#!/bin/bash
PWD=$(pwd)
cd ~/tracing/cpu_intensive/Debug

for i in `eval echo {1..$1}`
do
	./cpu_intensive $2 &
done

wait

cd $PWD
