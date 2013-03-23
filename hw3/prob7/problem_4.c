#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <assert.h>
#include <errno.h>


volatile int in_cs = 0;
volatile int is_stop = 0;
int n_thread = 0;


typedef struct _thread_data_t{
	int tid;
} thread_data_t;

thread_data_t *thr_data;

typedef struct _spin_lock_t{
	volatile int lock;
} spin_lock_t;

volatile spin_lock_t s;

/*
 * if (*ptr == old){
 * 	*ptr = new;
 * 	return old;
 * } else {
 * 	return *ptr;
 * }
 * */
static inline int atomic_cmpxchg (volatile int *ptr, int old, int new){
	int ret;
	asm volatile ("lock cmpxchgl %2, %1"
	: "=a" (ret), "+m" (*ptr)
	: "r" (new), "0" (old)
	: "memory"
	);
	return ret;
}

void spin_lock(volatile spin_lock_t *s){
	while ( atomic_cmpxchg(&s->lock, 0, 1) ){}	
	/*if s->lock is 1 (lock is acquired), then the atomic_cmpxchg() return 1.
 	 *else (lock is NOT acquired), then s->lock will be set to 1(new), and return 0, which lead to a breaking of the while 		*loop and the lock has been successfully accquired.
	 **/
}

void spin_unlock(volatile spin_lock_t *s){
	s->lock = 0;
}

void *get_in_line(void *arg){
	thread_data_t *data = (thread_data_t*) arg;
	unsigned int cnt = 0;
	int tid = data->tid;
	while (1){
		spin_lock(&s);
		/*critical section*/
		assert(in_cs==0);
		in_cs++;
		assert(in_cs==1);
		in_cs++;
		assert(in_cs==2);
		in_cs++;
		assert(in_cs==3);
		in_cs=0;
		spin_unlock(&s);
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
	
	s.lock = 0;	/*the lock is free at beginning*/
	thr_data = (thread_data_t*) malloc(n_thread*sizeof(thread_data_t));
	thread = (pthread_t*) malloc(n_thread*sizeof(pthread_t));

	if (thr_data==NULL || thread==NULL){
		fprintf(stderr, "Error allocating memory.\n");
		return -EXIT_FAILURE;
	}


	for (i = 0; i < n_thread; ++i){
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

	free((pthread_t*)thread);
	free(thr_data);
	thread = NULL;
	thr_data = NULL;
	
	return EXIT_SUCCESS;
}
