  // CSC456: Name -- Sweta Rout
  // CSC456: Student ID # -- 200206715
  // CSC456: I am implementing -- MERGE SORT {What parallel sorting algorithm using pthread}
  // CSC456: Feel free to modify any of the following. You can only turnin this file.

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <float.h>
#include <sys/time.h>
#include <pthread.h>
#include "mysort.h"
#include <unistd.h>
#include <iostream>
using namespace std;
pthread_mutex_t lock;
volatile int part=0;
int N;
int num_of_th=8;
volatile float *a;
typedef struct
{
	float* left;
	float* right;
}sort_data;
#define MAXT_IN_POOL 2
typedef void *threadpool;	//thread pool declaration
typedef void* (*worker_fn)(void *);
typedef struct worker{
	void* (*routine) (void*);
	void * arg;
	struct worker* next;
} workerThread;

typedef struct _threadpool_st {
	int num_threads;	//number of threads live
	int qsize;			//queue size
	pthread_t *threads;	//pointer to threads in the pool
	workerThread* qhead;		//queue head
	workerThread* qtail;		//queue tail
	pthread_mutex_t qlock;		//lock on the queue list
	pthread_cond_t is_not_empty;	//non empty and empty condidtion vairiables
	pthread_cond_t is_empty; //empty
	int shutdown;
	int unstable;	//cant add any more
	int num_of_jobs_finished;	//total number of jobs finished in the pool
} _threadpool;

void* do_work(threadpool p) {
	_threadpool * pool = (_threadpool *) p;
	workerThread* cur;	
	int k;

	while(1)
	{
		pool->qsize = pool->qsize;
		pthread_mutex_lock(&(pool->qlock));


		while( pool->qsize == 0) {
			if(pool->shutdown) {
				pthread_mutex_unlock(&(pool->qlock));
				pthread_exit(NULL);
			}

			pthread_mutex_unlock(&(pool->qlock));
			pthread_cond_wait(&(pool->is_not_empty),&(pool->qlock));

			//check to see if in shutdown mode.
			if(pool->shutdown) {
				pthread_mutex_unlock(&(pool->qlock));
				pthread_exit(NULL);
			}
		}
		cur = pool->qhead;
		pool->qsize--;
		if(pool->qsize == 0) 
		{
			pool->qhead = NULL;
			pool->qtail = NULL;
		}
		else 
			pool->qhead = cur->next;
		if(pool->qsize == 0 && ! pool->shutdown)
			pthread_cond_signal(&(pool->is_empty));
		pthread_mutex_unlock(&(pool->qlock));
		(cur->routine) (cur->arg);   //call merge_sort()
		pool->num_of_jobs_finished = pool->num_of_jobs_finished+1;
		free(cur);
	}
}

threadpool create_threadpool(int num_threads_in_pool)
{
	_threadpool *pool;
	int i;
	if ((num_threads_in_pool <= 0) || (num_threads_in_pool > MAXT_IN_POOL))
	    return NULL;
	pool = (_threadpool *) malloc(sizeof(_threadpool));
	if (pool == NULL)
		return NULL;
	pool->threads = (pthread_t*) malloc (sizeof(pthread_t) * num_threads_in_pool);
	if(!pool->threads)
        return NULL;	
	pool->num_threads = num_threads_in_pool;
	pool->qsize = 0;
	pool->qhead = NULL;
	pool->qtail = NULL;
	pool->shutdown = 0;
	pool->unstable = 0;
	pool->num_of_jobs_finished = 0;
	if(pthread_mutex_init(&pool->qlock,NULL))
		return NULL;
	if(pthread_cond_init(&(pool->is_empty),NULL))
		return NULL;
	if(pthread_cond_init(&(pool->is_not_empty),NULL))
		return NULL;
	for (i = 0;i < num_threads_in_pool;i++)
	{
		if(pthread_create(&(pool->threads[i]),NULL,do_work,pool))
			return NULL;
	}
	return (threadpool) pool;
}
void waitforjobsfinished(threadpool p)
{
	_threadpool * pool = (_threadpool *) p;
	while(true)
	{
		if(pool->num_of_jobs_finished == num_of_th)
			break;
	}
	return;
}
void addWork(threadpool mypool, worker_fn run,void *arg) 
{
	_threadpool *pool = (_threadpool *) mypool;
	pthread_mutex_lock(&(pool->qlock));
	workerThread * cur;
	int k;
	k = pool->qsize;
	cur = (workerThread*) malloc(sizeof(workerThread));
	if(cur == NULL)
		return;	
	cur->routine = run;
	cur->arg = arg;
	cur->next = NULL;

	if(pool->unstable)
	{ 
		free(cur);
		return;
	}
	if(pool->qsize == 0)
	{
		pool->qhead = cur;
		pool->qtail = cur;
		pthread_cond_signal(&(pool->is_not_empty)); 
	} 
	else
	{
		pool->qtail->next = cur;
		pool->qtail = cur;			
	}
	pool->qsize++;
	pthread_mutex_unlock(&(pool->qlock));
}

void destroy_threadpool(threadpool mypool)
{
	_threadpool *pool = (_threadpool *) mypool;
	void* nothing;
	pthread_mutex_lock(&(pool->qlock));
	pool->unstable = 1;
	while(pool->qsize != 0) {
		pthread_cond_wait(&(pool->is_empty),&(pool->qlock));
	}
	pool->shutdown = 1; 
	pthread_cond_broadcast(&(pool->is_not_empty)); 
	pthread_mutex_unlock(&(pool->qlock));
  
	for(int i=0;i < pool->num_threads;i++)
	{
		pthread_join(pool->threads[i],&nothing);
	}

	free(pool->threads);

	pthread_mutex_destroy(&(pool->qlock));
	pthread_cond_destroy(&(pool->is_empty));
	pthread_cond_destroy(&(pool->is_not_empty));
	return;
}
void merge(int low, int mid, int high,float *left_a,float *right_a)
{
    int n1 = mid - low + 1, n2 =high - mid, i, j;
    for (i = 0; i < n1; i++)
        left_a[i] = a[i + low];
    for (i = 0; i < n2; i++)
        right_a[i] = a[i + mid + 1];
    int k = low;
    i = j = 0;
    while (i < n1 && j < n2) {
        if (left_a[i] <= right_a[j])
            a[k++] = left_a[i++];
        else
            a[k++] = right_a[j++];
    }
    while (i < n1) {
        a[k++] = left_a[i++];
    }
     while (j < n2) {
        a[k++] = right_a[j++];
    }
}

void mergeSort( int l, int r,float *left,float* right)
{
    if (l < r)
    {
        int m = l+(r-l)/2;
        mergeSort(l, m,left,right);
        mergeSort(m+1, r,left,right);
        merge(l, m, r,left,right);
    }
}
 
void* merge_sort(void* val)
{
    int thread_part = part++;
	sort_data* tmp=(sort_data*)val;
	if(!tmp)
		return (void*)NULL;
    // calculating low and high
    int low = thread_part * (N / num_of_th);
    int high = (thread_part + 1) * (N / num_of_th) - 1;
    //fprintf(stderr,"low=%d and high=%d\n",low,high) ;
    int mid = low + (high - low) / 2;

    if (low < high) {
        mergeSort(low, mid,tmp->left,tmp->right);
        mergeSort(mid + 1, high,tmp->left,tmp->right);
        merge(low, mid, high,tmp->left,tmp->right);
    }
}


int pthread_sort(int num_of_elements, float *data)
{

	//struct timeval time_start, time_end_th,time_end_merge;
	threadpool tp;
	//gettimeofday(&time_start, NULL);   
	N=num_of_elements;
	int n=N;
	a=data;
	int m=n/2-1;
	//merge_sort((void*)NULL);
	tp=create_threadpool(2);
	for(int i=0;i<num_of_th;i++)
	{
		float *left_a = new float[num_of_elements/2];
		float * right_a = new float[num_of_elements/2];
		if(left_a==NULL || right_a==NULL)
			return 1;
		sort_data *tmp=new sort_data;
		tmp->left=left_a;tmp->right=right_a;
		addWork(tp,merge_sort,(void*)(tmp));
	}
	destroy_threadpool(tp);
	//sleep(30);
	//waitforjobsfinished(tp);
	float *left_a = new float[num_of_elements/2];
	float * right_a = new float[num_of_elements/2];
	//gettimeofday(&time_end_th, NULL);
	merge(0,m/4,m/2,left_a,right_a);
	merge(m/2+1,m/2+1+(m/2-1)/2,m,left_a,right_a);
	merge(m+1,m+1+(n-m-2)/4,m+1+(n-m-2)/2,left_a,right_a);
	merge(m+2+(n-m-2)/2,(m+2)/2+(n-1)/2+(n-m-2)/4+1,n-1,left_a,right_a);
	merge(0,m/2,m,left_a,right_a);
	merge(m+1,m+1+(n-m-2)/2,n-1,left_a,right_a);
	merge(0,(n-1)/2,n-1,left_a,right_a);
	//gettimeofday(&time_end_merge, NULL);
	//fprintf(stderr,"SORTING: %ld\n",((time_end_th.tv_sec * 1000000 + time_end_th.tv_usec) - (time_start.tv_sec * 1000000 + time_start.tv_usec)));
	//fprintf(stderr,"MERGING: %ld\n",((time_end_merge.tv_sec * 1000000 + time_end_merge.tv_usec) - (time_end_th.tv_sec * 1000000 + time_end_th.tv_usec)));
	//fprintf(stderr,"TOTAL: %ld\n",((time_end_merge.tv_sec * 1000000 + time_end_merge.tv_usec) - (time_start.tv_sec * 1000000 + time_start.tv_usec)));
	return 0;	
}
