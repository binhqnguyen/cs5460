#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#include <errno.h>

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
		fprintf (stderr,"Queue is full\n");
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
		fprintf(stderr,"Queue is empty\n");
		return 0;
	}
	if (q->tail == q->head){
		*v = q->value[q->head];
		q->tail = q->head = -1;
	} else{
		*v = q->value[q->head];
		q->head = (q->head+1) % 32;
	}
	//printf ("deq t/h = %d %d\n",q->tail,q->head);
	return 1; 
}

int main(){
	queue_t *queue;
	int i = 0;
	int v = 0;
	queue = create_queue();
	if (queue == NULL){
		fprintf(stderr,"Error creating queue\n");
		return -EXIT_FAILURE;
	}
	
	assert (deq(&v,queue)==0);	/*deque an empty queue, print "Queue is empty"*/

	for (i = 0; i < 32; ++i)
		assert (enq(5,queue)==1);	/*fill the queue with 5*/

	for (i = 0; i < 32; ++i)
		assert (queue->value[i]==5);	/*check the entire queue*/

	assert (enq(5, queue)==0);	/*enque a full queue, print "Queue is full"*/

	assert (deq(&v, queue)==1);	/*deque the first element*/

	assert (enq(6, queue)==1);	/*enque 6*/

	for (i = 0; i < 32; i++)
		assert(deq(&v,queue)==1);
	assert (v==6);	/*the last element should be 6*/
	
	return EXIT_SUCCESS;
}
