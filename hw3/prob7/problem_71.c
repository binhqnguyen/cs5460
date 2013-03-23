#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <math.h>

/*Enque and deque strategy:
 *(1) The value to be enqueued will start from 0, and increases after every enqueing.
 *(2) Dequeued element will be set to 0.
 *(3) After dequeing, thread compare the dequeued value with a counter starting from 0, and increase after each dequeing.
 * If the enqueing and dequeing are in the right order (no mutual exclusion violated):
 * + the enqueing will enque (0,1,2,3,4.....). 
 * + the dequeing will deque (0,1,2,3,4...) FIFO.
 */

 /*****NOTE****/
 /*Compile this find by at least 1 of the following compiler flags (WITHOUT a flag, the compiling won't success).
  *1. FENCE: mutex using fence.
  *2. SPIN_LOCK: mutex using spin lock.
  *3. FAIR_SPIN_LOCK: mutex using fair spin lock.
  *4. MUTEX: mutex using pthread mutex.
  *For example, mutex using FENCE: "gcc -DFENCE -Wall -Werror -Wextra problem_7.c -o problem_7 -lm -pthread"
  */  

/*********************Queue***************/
typedef struct _queue_t{
	int *value;
	int head, tail;
} queue_t;

queue_t *create_queue(){
	queue_t *ret = (queue_t*) malloc(1*sizeof(queue_t));
	int i = 0;
	ret->value = (int*) malloc(32*sizeof(int));
	if (ret->value == NULL){
		fprintf(stderr,"Error allocating memory in queue\n");
		return NULL;
	}
	for (i = 0; i < 32; ++i){
		ret->value[i] = 0;
	}
	ret->head = -1;
	ret->tail = -1;
	return ret;
}

int is_empty(queue_t *q){
	return (q->head == -1);
}

int is_full(queue_t *q){
	return (q->head == (q->tail+1)%32);
}

int enq(int v, queue_t *q){
	if ( is_full(q) ){
		//fprintf (stderr,"Queue is full\n");
		return 0;
	}
	if (q->tail==-1)
		q->head = 0;
	q->tail = (q->tail+1) % 32;
	q->value[q->tail] = v;
	//printf ("enq t/h = %d %d\n",q->tail,q->head);
	return 1;
}

int deq(int *v, queue_t *q){
	if ( is_empty(q) ){
		//fprintf(stderr,"Queue is empty\n");
		return 0;
	}
    *v = q->value[q->head];
    /*Dequeued element will be set to 0.
     *Though not required, this is used to test for queue behavior.
     */
    //q->value[q->head] = 0;
	if (q->tail == q->head){    /*dequeing last element*/
		q->tail = q->head = -1; /*reset head and tail to empty the queue*/
	} else{
		q->head = (q->head+1) % 32; /*advance the head*/
	}
	//printf ("deq t/h = %d %d\n",q->tail,q->head);
	return 1; 
}
/******************************Queue**************************/

int n_producer = 0;
int n_consumer = 0;
int second = 0;
int is_stop = 0;
volatile int enque_v = 0;
volatile int deque_v = 0;
queue_t *queue;
int *cnt;
double producer_stdev = 0;    /*producers' standard deviation*/
double consumer_stdev = 0;    /*cosumers' standard deviation*/


typedef struct _thread_data_t{
	int tid;
} thread_data_t;

/***********************Pthread mutex*************************/
#ifdef MUTEX
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void *consume(void *arg){
    thread_data_t *data = (thread_data_t*) arg;
    int tid = data->tid;
    int v = 0;
    while (1) {
        pthread_mutex_lock(&mutex);
        /*Dequeing testing strategy:
         *If multiple threads are in the critical section in the same time
         *there is a chance that some of the threads will dequeue
         *a ALREADY dequeued element, and these threads will get a 0 (instead of 1 for a
         *successful dequeing).
         */
        if (deq(&v, queue)==1){ /*if successfully dequeued*/
            cnt[tid]++;  /*increase the counter*/
            assert(v==1);   /*check for dequeued value:
                             *1 means correct dequeuing.
                             *0 means incorrect dequeing (the element is previously dequed).
                             **/
        }
        pthread_mutex_unlock(&mutex);
        if (is_stop){
            printf("Consumer %d consumed %d items.\n",tid,cnt[tid]);
            pthread_exit(NULL);
        }
    }
}

void *produce(void *arg){
    thread_data_t *data = (thread_data_t*) arg;
    int tid = data->tid;
    while (1) {
        pthread_mutex_lock(&mutex);
        /*Enqueing testing strategy:
         *If multiple threads are in the critical section in the same time,
         *the enque_v will NOT equal 1 but some value larger than 1.
         */
        enque_v++;  /*1 will be enqueued*/
        cnt[tid] += enq(enque_v, queue);
        enque_v = 0;
        pthread_mutex_unlock(&mutex);
        if (is_stop){
            printf("Producer %d produced %d items.\n",tid,cnt[tid]);
            pthread_exit(NULL);
        }
    }
}
#endif

/***********************Pthread mutex*************************/



/****************************Fair Spin_lock*******************/
#ifdef FAIR_SPIN_LOCK

typedef struct _spin_lock_t {
	int waiting;
	int serving;
} spin_lock_t;

spin_lock_t s;

static inline int atomic_xadd (volatile int *ptr){
	register int val __asm__("eax") = 1;
	asm volatile ("lock xaddl %0, %1"
                  : "+r" (val)
                  : "m" (*ptr)
                  : "memory"
                  );
	return val;
}

void spin_lock(spin_lock_t *s){
	int my_turn = atomic_xadd(&s->waiting);
	while (my_turn != s->serving){
	}
}
void spin_unlock(spin_lock_t *s){
    atomic_xadd(&s->serving);
}

#endif
/*************************Fair Spin_lock************************/


/*************************Spin Lock***********************/
#ifdef SPIN_LOCK
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
 	 *else (lock is NOT acquired), then s->lock will be set to 1(new), and return 0, which lead to a
         *breaking of the while loop and the lock has been successfully accquired.
	 **/
}

void spin_unlock(volatile spin_lock_t *s){
	s->lock = 0;
}
#endif
/************************Spin lock*************************/


/****************************Fair/Unfair Spin_lock*********************/
#if defined (SPIN_LOCK) || defined(FAIR_SPIN_LOCK)
void *consume(void *arg){
	thread_data_t *data = (thread_data_t*) arg;
	int tid = data->tid;
    int v = 0;
	while (1){
		spin_lock(&s);
		/*critical section*/
        if (deq(&v, queue)==1){ /*if successfully dequeued*/
            cnt[tid]++;  /*increase the counter*/
            assert(v==1);   
        }
		spin_unlock(&s);
		if (is_stop){
			printf("Consumer %d consumed %d items.\n",tid,cnt[tid]);
			pthread_exit(NULL);
		}
	}
}

void *produce(void *arg){
	thread_data_t *data = (thread_data_t*) arg;
	int tid = data->tid;
	while (1){
		spin_lock(&s);
		/*critical section*/
        enque_v++;
        cnt[tid] += enq(enque_v, queue);
        enque_v = 0;
		spin_unlock(&s);
		if (is_stop){
			printf("Producer %d produced %d items.\n",tid,cnt[tid]);
			pthread_exit(NULL);
		}
	}
}
#endif
/****************************Fair/Unfair Spin_lock*********************/



/*************************Fence***********************/
#ifdef FENCE

int *choosing;
int *ticket;

void mfence(void){
	asm volatile ("mfence" : : : "memory");
}

/*return max value of an array*/
int max(volatile int* arr){
	int i = 0;
	int max = arr[0];
	for ( i = 1; i < (n_consumer+n_producer); ++i){
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
	/* 2 fences are added around the portion where users are trying to acquire ticket numbers.
 	 * These fences force the ticket number acquiring process to be SEQUENTIAL, this means ALL previous memory accesses
 	 * have to be completed before go into the fence boundary.
 	 * This will enable the sequential ticket selection, users will share a consistent ticket array.
 	 * We don't need other fences because the memory LOAD/STORE is required to be sequential ONLY in this place. Other
 	 * places memory is just LOAD(read) but not STORE(write).
 	 * */
	mfence();
	ticket[tid] = max(ticket)+1;
	mfence();
	choosing[tid] = 0;
	/* thread checks if it has the highest priority*/
	for (i = 0; i < (n_consumer+n_producer); ++i){
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

void *consume(void *arg){
	thread_data_t *data = (thread_data_t*) arg;
	int tid = data->tid;
    int v = 0;
	while (1){
		lock(tid);
		/*critical section*/
        if (deq(&v, queue)==1){ /*if successfully dequeued*/
            cnt[tid]++;  /*increase the counter*/
            assert(v==deque_v);
	    deque_v++;
        }
		unlock(tid);
		if (is_stop){
			printf("Consumer %d consumed %d items.\n",tid,cnt[tid]);
			pthread_exit(NULL);
		}
	}
}

void *produce(void *arg){
	thread_data_t *data = (thread_data_t*) arg;
	int tid = data->tid;
	while (1){
		lock(tid);
		/*critical section*/
        if ( enq(enque_v, queue) == 1){
		cnt[tid]++;
		enque_v++;
	}
        unlock(tid);
		if (is_stop){
			printf("Producer %d produced %d items.\n",tid,cnt[tid]);
			pthread_exit(NULL);
		}
	}
}

#endif

/*************************Fence**************************/



/*
 *Compute standard deviations
 */
void compute_stdev(){
    float sum = 0;
    float producer_avg = 0;
    float consumer_avg = 0;
    int i = 0;

    /*Calculate average of producers*/
    sum = 0;
    for (i = 0; i < n_producer; ++i) {
        sum += cnt[i];
    }
    producer_avg = sum/n_producer;
    
    /*Calculate average of consumers*/
    sum = 0;
    for (i = n_producer; i < (n_consumer+n_producer); ++i) {
        sum += cnt[i];
    }
    consumer_avg = sum/n_consumer;
    
    /*Calculate standard deviation for producers*/
    sum = 0;
    for (i = 0; i < n_producer; ++i) {
        sum += (cnt[i] - producer_avg)*(cnt[i] - producer_avg); /*sum = Sigma((value - average)^2)*/
    }
    sum = sum/n_producer;
    producer_stdev = sqrt(sum); /*Take squared root of the sum to get the stdev*/
    
    /*Calculate standard deviation for consumers*/
    sum = 0;
    for (i = n_producer; i < (n_producer+n_consumer); ++i) {
        sum += (cnt[i] - consumer_avg)*(cnt[i] - consumer_avg); /*sum = Sigma((value - average)^2)*/
    }
    sum = sum/n_consumer;
    consumer_stdev = sqrt(sum); /*Take squared root of the sum to get the stdev*/
}

/***********************main***********************/

int main(int agrc, char** agrv){
	int i = 0;
	pthread_t *consumer;
    pthread_t *producer;
    thread_data_t *thr_data;
    unsigned long n_item_consumed = 0;
    unsigned long n_item_produced = 0;


    
	if (agrc != 4){
		printf("Usage: ./problem_7 <number of producers> <number of consumers> <run time (seconds)>.\n");
		return -E2BIG;	/*argument list too long*/
	}
    
    sscanf(agrv[1],"%d",&n_producer);
    sscanf(agrv[2],"%d",&n_consumer);
    sscanf(agrv[3],"%d",&second);
    
	/*check for argument*/
	if (n_producer == -1 || n_consumer == -1 || second == -1){	/*not integer*/
		printf("Invalid arguments %d %d %d (not integer).\n",n_producer,n_consumer,second);
		printf("Usage: ./problem_7 <number of producers> <number of consumers> <run time (seconds)>.\n");
		return -EINVAL;	/*invalid arguments*/
	}
    
#ifdef FAIR_SPIN_LOCK
    s.waiting = 0;
    s.serving = 0;
#endif
    
#ifdef FENCE
    choosing = (int*) malloc((n_consumer+n_producer)*sizeof(int));
    ticket = (int*) malloc((n_consumer+n_producer)*sizeof(int));
#endif
    
    queue = create_queue();
    cnt = (int*) malloc ((n_consumer+n_producer)*sizeof(unsigned int));
    consumer = (pthread_t*) malloc(n_consumer*sizeof(pthread_t));
    producer = (pthread_t*) malloc(n_producer*sizeof(pthread_t));
    thr_data = (thread_data_t*) malloc((n_producer+n_consumer)*sizeof(thread_data_t));
    
    if (cnt == NULL || consumer == NULL || producer == NULL || thr_data==NULL){
	fprintf(stderr, "Error allocating memory.\n");
	return -EXIT_FAILURE;
    }

    
    for (i = 0; i < (n_consumer+n_producer); ++i){
	thr_data[i].tid = 0;
        cnt[i] = 0;
    }
    int rc = 0;
    
    for (i = 0; i < n_producer; ++i){   /*producers' id: 0 to n_producer*/
	thr_data[i].tid = i;
	if ( (rc = pthread_create(&producer[i], NULL, produce , &thr_data[i]))){ /*creat producers*/
		fprintf(stderr, "error: pthread_create, rc: %d\n", rc);
		return -EXIT_FAILURE;
	}
    }
    
    for (i = n_producer; i < (n_producer+n_consumer); ++i){
    /*consumers' id: n_producer to (n_producer+n_consumer)*/
	thr_data[i].tid = i;
	if ( (rc = pthread_create(&consumer[i-n_producer], NULL, consume , &thr_data[i]))){ /*creat consumers*/
		fprintf(stderr, "error: pthread_create, rc: %d\n", rc);
		return -EXIT_FAILURE;
	}
    }
	
    sleep(second);
    is_stop = 1;
    
    /*joining*/
    for (i = 0; i < n_producer; ++i){
		pthread_join(producer[i], NULL);
        n_item_produced += cnt[i];
    }
    for (i = 0; i < n_consumer; ++i){
		pthread_join(consumer[i], NULL);
        n_item_consumed += cnt[n_producer+i];
    }
    
    /*
     *Testing the queue. If mutual exclusion is violated,
     *the queue will likely contain element other than 0 (empty) or 1 (filled).
     */
    //for (i = 0; i < 32; ++i){
    //    assert(queue->value[i]==1 || queue->value[i]==0);
    //}
    
    compute_stdev();
    
    printf("<1>***Producers standard deviation: %.0f\n",producer_stdev);
    printf("<2>***Consumers standard deviation: %.0f\n",consumer_stdev);
    printf("<3>***Total items produced/consumed per second %lu %lu\n", (n_item_produced/second), (n_item_consumed/second));

    
    free((pthread_t*)consumer);
    free((pthread_t*)producer);
    free((thread_data_t*)thr_data);
	free((queue_t*)queue);
	free((int*)cnt);
    consumer = NULL;
    producer = NULL;
    thr_data = NULL;
	queue = NULL;
	cnt = NULL;
#ifdef FENCE
    free((int*)choosing);
    free((int*)ticket);
    choosing = NULL;
    ticket = NULL;
#endif
	
    return EXIT_SUCCESS;
}

