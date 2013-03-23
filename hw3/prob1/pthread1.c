#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#define NUM_THREADS 10

#define LEN(x) (sizeof(x)/sizeof(x[0])) 

volatile uint32_t in_cs = 0;
volatile uint32_t *ticket;
volatile bool *choosing;

typedef struct _thread_data_t{
	int tid;
} thread_data_t;

/*
 * Implementing lock function for Lamport's Barkery
 */
void lock(uint32_t tid){
	printf("Thread %d getting into lock function.\n", tid);
	/* thread picking a number*/
	choosing[tid] = true;
	ticket[tid] = max(ticket)+1;
	choosing[tid] = false;
	/* thread checks if it has the highest priority*/
	for (uint32_t i = 0; i < LEN(ticket); ++i){
		if (i != tid){
			while (choosing[i]) {}	/*spin if the other thread is picking a number*/
			while( (ticket[i] != 0)&&( (ticket[tid] > ticket[i]) || (ticket[tid] == ticket[i] && tid > i) ) ){} /*spinning on the currently being serviced customer, until my turn comes*/
		}
	}
	/*critical section will be after this*/
}

void unlock(uint32_t tid){
	printf("Thread %d unlocking...\n",tid);
	ticket[tid] = 0;	/*erase my ticket, other threads will not wait on me on the 2nd while*/
}

void *get_in_line(void *arg){
	thread_data_t *data = (thread_data_t*) arg;
	uint32_t tid = data->tid;
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
	pthread_exit(NULL);
}

int main(){
	
	pthread_t thr[NUM_THREADS];
	int i, rc;

	shared_x = 0;
	pthread_mutex_init(&lock_x, NULL);

	thread_data_t thr_data[NUM_THREADS];
	for (i = 0; i < NUM_THREADS; ++i){
		thr_data[i].tid = i;
		thr_data[i].stuff = (i+1)*NUM_THREADS;
		if ( (rc = pthread_create(&thr[i], NULL, thr_func, &thr_data[i])) ){
			fprintf(stderr, "error: pthread_create, rc: %d\n",rc);
			return EXIT_FAILURE;
			
		} 
	}
	for (i = 0; i < NUM_THREADS; ++i){
		pthread_join(thr[i], NULL);
	//	printf("%d\n",pthread_join(thr[i],NULL) );
	}
	return EXIT_SUCCESS;
}
