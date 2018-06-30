/*
 * main.c
 *
 *  Created on: Jun 30, 2018
 *      Author: azhari
 */


#include <stdlib.h>
#include <stdio.h>

int main(int argc, char **argv){
	long long i=0;
	long long j=0;
	long long k=0;
	long long MaxIter = 1000;
	if (argc == 2){
		MaxIter = atoll(argv[1]);
		printf("Maximum Iterations = %lld\n",MaxIter);
	}
	double sum=0;
	for (i=0;i < MaxIter; i++){
		for (j=0; j< MaxIter; j++){
			for (k=0; k < MaxIter; k++){
				sum = sum+rand()*rand()*rand()*i*j*k;
			}
		}
	}
	return 0;
}
