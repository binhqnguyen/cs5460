#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <assert.h>
#include <errno.h>


volatile int in_cs = 0;
volatile int *ticket;
volatile int *choosing;
volatile int is_stop = 0;
int n_thread = 0;


typedef struct _thread_data_t{
	int tid;
} thread_data_t;

thread_data_t *thr_data;

/*return max value of an array*/
int max(volatile int* arr){
	int i = 0;
	int max = arr[0];
	for ( i = 1; i < n_thread; ++i){
		if (arr[i] > max)
			max = arr[i];
	}
	return max;
}

/*
 * Implementing lock function for Lamport's Barkery
 */
void lock(int tid){
	int i = 0;
	/* thread picking a number*/
	choosing[tid] = 1;
	ticket[tid] = max(ticket)+1;
	choosing[tid] = 0;
	/* thread checks if it has the highest priority*/
	for (i = 0; i < n_thread; ++i){
		if (i != tid){
			while (choosing[i]==1) {}	/*spin if the other thread is picking a number*/
			while( (ticket[i] != 0) && ( ( ticket[tid] > ticket[i] ) || ( (ticket[tid] == ticket[i]) && (tid > i)) ) ){} /*spinning on the currently being serviced customer, until my turn comes*/
		}
	}
	/*critical section will be after this*/
}

void unlock(int tid){
	ticket[tid] = 0;	/*erase my ticket, other threads will not wait on me on the 2nd while*/
}

void *get_in_line(void *arg){
	thread_data_t *data = (thread_data_t*) arg;
	unsigned int cnt = 0;
	int tid = data->tid;
	while (1){
		lock(tid);
		/*critical section*/
		assert(in_cs==0);
		in_cs++;
		assert(in_cs==1);
		in_cs++;
		assert(in_cs==2);
		in_cs++;
		assert(in_cs==3);
		in_cs=0;
		unlock(tid);
		cnt++;
		if (is_stop){
			printf("Thread %d entered cs %d times.\n",tid,cnt);
			pthread_exit(NULL);
		}
	}
}

int main(int agrc, char** agrv){
	int second = 0;
	int i = 0;
	pthread_t *thread;

	if (agrc != 3){
		printf("Usage: ./problem1 <number of threads> <run time (seconds)>.");
		return -E2BIG;	/*argument list too long*/
	}

	sscanf(agrv[1],"%d",&n_thread);
	sscanf(agrv[2],"%d",&second);

	/*check for argument*/
	if (n_thread == -1 || second == -1){	/*not integer*/
		printf("Invalid arguments %d %d (not integer).\n",n_thread,second);
		printf("Usage: ./problem1 <number of threads> <run time (seconds)>.");
		return -EINVAL;	/*invalid arguments*/
	}
	
	ticket = (int*) malloc(n_thread*sizeof(int));
	choosing = (int*) malloc(n_thread*sizeof(int));
	thr_data = (thread_data_t*) malloc(n_thread*sizeof(thread_data_t));
	thread = (pthread_t*) malloc(n_thread*sizeof(pthread_t));

	if (ticket==NULL || choosing==NULL || thr_data==NULL || thread==NULL){
		fprintf(stderr, "Error allocating memory.\n");
		return -EXIT_FAILURE;
	}


	for (i = 0; i < n_thread; ++i){
		ticket[i] = 0;
		choosing[i] = 0;
		thr_data[i].tid = 0;
	}
	int rc = 0;
	for (i = 0; i < n_thread; ++i){
		thr_data[i].tid = i;
		if ( (rc = pthread_create(&thread[i], NULL, get_in_line, &thr_data[i]))){ /*creat threads*/
			fprintf(stderr, "error: pthread_create, rc: %d\n", rc);
			return -EXIT_FAILURE;
		}
	}
	
	sleep(second);
	is_stop = 1;

	for (i = 0; i < n_thread; ++i){
		pthread_join(thread[i], NULL);
	}

	free((int*)ticket);
	free((int*)choosing);
	free(thr_data);
	ticket = NULL;
	choosing = NULL;
	thr_data = NULL;
	
	return EXIT_SUCCESS;
}
