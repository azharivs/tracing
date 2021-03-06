#!/bin/bash
echo "Usage:"
echo "./cpu_htrc.sh [tracing_session_name] [number_of_processes_to_spawn] [max_iterations] [user@vm_ip]"

if [ "$1" = "" ] 
then
	exit 
fi

if [ "$2" == "" ] 
then 
	exit 
fi

if [ "$3" == "" ] 
then 
	exit 
fi

if [ "$4" == "" ]
then 
	exit
fi

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

#ssh script that spawns application to be traced appropriately on guest
ssh $4 "nohup ~/tracing/tracing_tools/cpu_intensive.sh $2 $3 "
wait
#stop tracing
sleep 1
lttng stop
sleep 1
lttng destroy

