/*
 * mc.c
 *
 *  Created on: May 23, 2018
 *      Author: azhari
 *
 * Two threads contending over a mutex
 * This is to see how a critical path of execution can be formed between two threads
 * working in a producer consumer manner.
 * I will investigate how asynchronous processing may affect the detection of a critical path.
 * entry_futex corresponds to locking a mutex. If thread is blocked then we will get a sched_out
 * after this, other wise we get exit_futex.
 *
 * Interesting patterns are:
 *
 * -- entry|exit futex enclosed in sched_wake meaning that some mutex is released or cond var signalled
 *    and the mutex/cond var is waited upon
 *
 * -- entry|exit futex back to back with no wakeup and no sched_out meaning:
 *    	-- either lock is acquired
 *    	-- or released/signalled but for which no thread is waiting upon
 *
 * -- entry_futex followed by sched_out meaning that mutex or cond var acquire resulted in blocking
 *
 * I suggest the following approach to detect potential mutex dependencies among threads:
 *
 * Keep track of uaddr for all futexes
 * Create a list of threads which are related with a common futex uaddr being accessed
 * Connect two threads with a link weight equal to the number of times one was waken by the other
 * This creates a directional graph among threads. Edges are also labeled with the futext uaddr over
 * which this transaction happened.
 *
 * Or we can use three matrices of ThreadsxMutexes where cell_ij represents the number of times
 * Thread i has accessed Mutex j, blocked on Mutex j, was unblocked through Mutex j, unblocked other
 * thread (k) through Mutex j.
 *
 * TODO: Build a potential worst case critical execution path containing the most number of
 *       interdependent threads.
 *
 * TODO: Visualize time in critical section with different color (each mutex has a color/name)
 *
 * TODO: Good to know how often a thread is blocked on a mutex and how often it acquires without blocking
 * TODO: Which thread unblocks a waiting thread?
 * TODO: what is the meaning of exit_futex return value? 0/1/-11?
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/syscall.h>

#define MAX_COUNT 100
#define BUSY_LOOP_ITER 1000

pthread_mutex_t count_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
int count = 0;

void *func1();
void *func2();
int busy_wait(int iter);

//increment even to become odd and print
void *func1(){
	for (;;){
		//get an even count and increment to become odd then print
		pthread_mutex_lock(&count_lock);
		//entry_futex uaddr=xxxx712, uaddr2=0
		//sched_out: meaning the lock was busy and thread is blocked
		//....
		//sched_in:
		//exit_futex uaddr=xxxx712, uaddr2=0, ret=0
		while (count % 2 == 1){//while condition is false (count is odd)
			//wait for count to become even again by other thread
			pthread_cond_wait(&cond, &count_lock);
			//entry_futex uaddr=xxxx780, uaddr2=xxxx552
			//sched_out
			//blocked ... until wakeup and waking from func2()
			//sched_in
			//exit_futex uaddr=xxxx780, uaddr2=xxxx552, ret=0
		}
		//increment count to become odd
		count ++;
		//printf("F1Count = %d\n",count);
		//busy wait doing something
		busy_wait(BUSY_LOOP_ITER);
		pthread_mutex_unlock(&count_lock);
		//entry_futex uaddr=xxxx712, uaddr2=0
		//sched_waking func2 signals other thread which is blocked on this mutex
		//sched_wakeup func2
		//exit_futex uaddr=xxxx712, uaddr2=0, ret=1


		if (count >= MAX_COUNT){
			pid_t tid = syscall(SYS_gettid);
			printf("Func1 TID: %d\n",tid);
			return NULL;
		}
	}
}

//increment odd to become even and print
void *func2(){
	for (;;){
		pthread_mutex_lock(&count_lock);
		//entry|exit_futex uaddr=xxxx712, uaddr2=xxxx712
		//exit only upon acquiring of lock
		if (count % 2 == 1){//if my turn to increment? (count is odd)
			//increment count to become even
			count ++;
			//printf("F2Count = %d\n",count);
			//busy wait doing something so that the other thread is definitely blocked
			//by kernel and not busy waiting
			busy_wait(BUSY_LOOP_ITER);
			//signal other thread
			pthread_cond_signal(&cond);
			//entry_futex uaddr=xxxx780 uaddr2=xxxx776
			//sched_waking func1
			//sched_wakeup func1
			//exit_futex uaddr=xxxx780 uaddr2=xxxx776
		}
		pthread_mutex_unlock(&count_lock);
		//entry_futex uaddr=xxxx712, uaddr2=0
		//exit_futex uaddr=xxxx712, uaddr2=0, ret=?
		//executed back to back without any sched_waking meaning no thread is blocked on this mutex
		//TODO: Good to know how often a thread is blocked on a mutex

		if (count >= MAX_COUNT){
			pid_t tid = syscall(SYS_gettid);
			printf("Func2 TID: %d\n",tid);
			return NULL;
		}
	}
}

//busy wait in double nested for loops for iter iterations
int busy_wait(int iter){
	double res = 0;
	for (int i=iter;i>0;i--){
		for (int j=iter;j>0;j--)
			res = res + i*j;
	}
	return res;
}

main(){
	pthread_t t1,t2;

	pthread_create( &t1, NULL, func1, NULL);
	pthread_create( &t2, NULL, func2, NULL);

	pthread_join( t1, NULL);
	pthread_join( t2, NULL);

	printf("Done!\n");

	exit(EXIT_SUCCESS);
}
