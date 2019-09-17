//Amit Ashkenazy
//204709356

#include <malloc.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>

#include "threadPool.h"
#include "osqueue.h"

void error(ThreadPool *threadPool) {
    if (threadPool != NULL) { tpDestroy(threadPool, 0); }
    write(stderr, "Error in system call", 20);
    exit(-1);
}

void* threadExecution(void* arg) {
    ThreadPool *threadPool = arg;

    while (1) {
        pthread_mutex_lock(&threadPool->lock);

        if (threadPool->waitForTasks == 0) {
            pthread_mutex_unlock(&threadPool->lock);
            break;
        }

        else if (!osIsQueueEmpty(threadPool->taskQueue)) {
            Task *task = osDequeue(threadPool->taskQueue);
            pthread_mutex_unlock(&threadPool->lock);
            task->computeFunc(task->param);
            free(task);
        }

        else if (threadPool->destroyCalled == 1) {
            pthread_mutex_unlock(&threadPool->lock);
            break;
        }

        else {
            pthread_cond_wait(&threadPool->executionCondition, &threadPool->lock);
            pthread_mutex_unlock(&threadPool->lock);
        }
    }

    //pthread_exit(NULL);
    return NULL;
}

ThreadPool* tpCreate(int numOfThreads) {
    int threadIndex;

    if (numOfThreads <= 0) { return NULL; }

    ThreadPool* threadPool = (ThreadPool*) malloc(sizeof(ThreadPool));
    if (threadPool == NULL) { error(threadPool); }

    threadPool->receiveNewTasks = 1;
    threadPool->destroyCalled = 0;
    threadPool->waitForTasks = 1;
    threadPool->numOfThreads = numOfThreads;
    threadPool->taskQueue = osCreateQueue();
    threadPool->allThreads = osCreateQueue();

    for (threadIndex = 0; threadIndex < numOfThreads; ++threadIndex) {
        pthread_t *pthread = (pthread_t*) malloc(sizeof(pthread_t));
        if (pthread == NULL) { error(threadPool); }
        osEnqueue(threadPool->allThreads, pthread);
        if (pthread_create(pthread, NULL, threadExecution, threadPool))
        {
            printf("Error creating pthread\n");
        }
    }

    return threadPool;
}

void tpDestroy(ThreadPool* threadPool, int shouldWaitForTasks) {
    int threadIndex;

    threadPool->receiveNewTasks = 0;
    threadPool->destroyCalled = 1;
    threadPool->waitForTasks = shouldWaitForTasks;

    if (shouldWaitForTasks == 0) {
        while (threadPool->taskQueue->head != NULL) {
            free(osDequeue(threadPool->taskQueue));
        }
    }

    pthread_cond_broadcast(&threadPool->executionCondition);

    for (threadIndex = 0; threadIndex < threadPool->numOfThreads; ++threadIndex) {
        pthread_t *pthread = osDequeue(threadPool->allThreads);
        pthread_join(*pthread, NULL);
        free(pthread);
    }

    free(threadPool->taskQueue);
    free(threadPool->allThreads);
    free(threadPool);
}

int tpInsertTask(ThreadPool* threadPool, void (*computeFunc) (void *), void* param) {
    if (threadPool->receiveNewTasks == 0) { return -1; }

    Task *newTask = (Task*) malloc(sizeof(Task));
    if (newTask == NULL) { error(threadPool); }

    newTask->computeFunc = computeFunc;
    newTask->param = param;
    osEnqueue(threadPool->taskQueue, newTask);

    pthread_cond_signal(&threadPool->executionCondition);
    pthread_mutex_unlock(&threadPool->lock);
    return 0;
}
