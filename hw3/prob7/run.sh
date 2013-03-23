#!/bin/bash

N_THREAD=1
TIME=15
RUNTIME=1

cd ~/hw3/prob7
while [ $RUNTIME -le 5 ]; do
	let N_THREAD=1;
	echo "*****runtime $RUNTIME ********"
	while [ $N_THREAD -le 6 ]; do
		echo "$N_THREAD $TIME FENCE"
		gcc -DFENCE -o2 -Wall -Wextra -Werror problem_7_n.c -o problem_7_n -lm -pthread
		./problem_7_n $N_THREAD $N_THREAD $TIME
		echo "*********************"
		echo "$N_THREAD $TIME FAIR SPIN LOCK"
		gcc -DFAIR_SPIN_LOCK -o2 -Wall -Wextra -Werror problem_7_n.c -o problem_7_n -lm -pthread
		./problem_7_n $N_THREAD $N_THREAD $TIME
		echo "********************\n"
		echo "$N_THREAD $TIME SPIN LOCK"
		gcc -DSPIN_LOCK -o2 -Wall -Wextra -Werror problem_7_n.c -o problem_7_n -lm -pthread
		./problem_7_n $N_THREAD $N_THREAD $TIME
		echo "********************\n"
		echo "$N_THREAD $TIME MUTEX"
		gcc -DMUTEX -o2 -Wall -Wextra -Werror problem_7_n.c -o problem_7_n -lm -pthread
		./problem_7_n $N_THREAD $N_THREAD $TIME
		echo "*******************\n"
		let N_THREAD=N_THREAD+1
	done
	echo "*****runtime********\n"
	let RUNTIME=RUNTIME+1
done	

