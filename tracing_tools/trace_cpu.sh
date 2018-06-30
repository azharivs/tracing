#!/bin/bash

#create lttng session and initialize 
lttng create $1
lttng enable-channel ss -k --tracefile-size 10M --subbuf-size 4M --num-subbuf 32
lttng add-context --kernel --channel ss --type tid  
lttng add-context --kernel --channel ss --type pid

#initiaize events 
lttng enable-event -c ss -k -a 

#start tracing
lttng start
sleep 1

#start application to be traced
../cpu_intensive/Debug/cpu_intensive $2

#stop tracing
sleep 1
lttng stop
sleep 1
lttng destroy

