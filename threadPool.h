//Amit Ashkenazy
//204709356

#ifndef __THREAD_POOL__
#define __THREAD_POOL__

#include <pthread.h>
#include "osqueue.h"

typedef struct task {
    void (*computeFunc) (void *);
    void* param;
}Task;

typedef struct thread_pool
{
    int receiveNewTasks;
    int destroyCalled;
    int waitForTasks;
    int numOfThreads;
    OSQueue* allThreads;
    OSQueue* taskQueue;
    pthread_mutex_t lock;
    pthread_cond_t executionCondition;
}ThreadPool;

ThreadPool* tpCreate(int numOfThreads);

void tpDestroy(ThreadPool* threadPool, int shouldWaitForTasks);

int tpInsertTask(ThreadPool* threadPool, void (*computeFunc) (void *), void* param);

#endif
