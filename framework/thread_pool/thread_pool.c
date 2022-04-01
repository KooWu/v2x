#include "thread_pool.h"
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include "my_log.h"
#include "linked_list.h"

#define THREAD_POOL_NUM_MAX 7

typedef struct _ThreadPoolJobNodeInfo {
	struct _ThreadPoolJobNodeInfo *prev;
	struct _ThreadPoolJobNodeInfo *next;
	ThreadPoolJobInfo jobInfo;
} ThreadPoolJobNodeInfo;

typedef struct {
	bool isRunning;
	int32_t curJobNum;
	int32_t maxThreadNum;
	int32_t idleThreadNum;
	pthread_t threadId[THREAD_POOL_NUM_MAX];
	pthread_mutex_t lock;
	sem_t sem;
} ThreadPoolInfo;

static bool g_isInit = false;
static pthread_mutex_t g_initMutex = PTHREAD_MUTEX_INITIALIZER;
static ThreadPoolInfo g_threadPoolInfo;
static DoublyLinkedListNode g_jobListHead; 

static void ThreadPoolExitHandle(int32_t num)
{
    dbg("ThreadPoolExitHandle num: %d\n", num);
	g_threadPoolInfo.isRunning = false;
	int32_t idx;
	for (idx = 0; idx < num; idx++) {
		sem_post(&g_threadPoolInfo.sem);
		dbg("sem_post\n");
		sleep(1);
		pthread_join(g_threadPoolInfo.threadId[num], NULL);
		dbg("pthread_join\n");
	}
	pthread_mutex_destroy(&g_threadPoolInfo.lock);
	sem_destroy(&g_threadPoolInfo.sem);
}

static void *PthreadPoolProcessThread(void *arg)
{
	dbg("thread run %ld\n", pthread_self());
	ThreadPoolJobNodeInfo *cur = NULL;
    DoublyLinkedListNode curHead; 
	while (g_threadPoolInfo.isRunning) {
		pthread_mutex_lock(&g_threadPoolInfo.lock);
		if (g_threadPoolInfo.idleThreadNum != g_threadPoolInfo.maxThreadNum) {
			g_threadPoolInfo.idleThreadNum++;
		}
		pthread_mutex_unlock(&g_threadPoolInfo.lock);
		dbg("thread wait %ld\n", pthread_self());
		sem_wait(&g_threadPoolInfo.sem);
		pthread_mutex_lock(&g_threadPoolInfo.lock);
        dbg("thread wake up %ld\n", pthread_self());
		g_threadPoolInfo.idleThreadNum--;
        int32_t ret = DeleteTailDoublyLinkedListNode(&g_jobListHead, &curHead);
		if (ret != 0) {
			pthread_mutex_unlock(&g_threadPoolInfo.lock);
			continue;
		}
		g_threadPoolInfo.curJobNum--;
		pthread_mutex_unlock(&g_threadPoolInfo.lock);
        cur = (ThreadPoolJobNodeInfo *)curHead.nodeData;
        if (cur != NULL) {
            cur->jobInfo.jobFunc(cur->jobInfo.arg);
		    free(curHead.nodeData);
        }
		dbg("thread %ld done job\n", pthread_self());
	}
	dbg("thread done %ld\n", pthread_self());
	return NULL;
}

int32_t ThreadPoolInit(int32_t threadNum)
{
	if ((threadNum <= 0) || (threadNum >= THREAD_POOL_NUM_MAX)) {
		dbg("invalid threadNum : %d\n", threadNum);
		return -3;
	}

	pthread_mutex_lock(&g_initMutex);
	if (g_isInit) {
		dbg("is inited\n");
		pthread_mutex_unlock(&g_initMutex);
		return 0;
	}

	int32_t ret;
	do {
        (void)InitDoublyLinkedListNode(&g_jobListHead);
		g_threadPoolInfo.isRunning = true;
		g_threadPoolInfo.curJobNum = 0;
		g_threadPoolInfo.maxThreadNum = threadNum;
		g_threadPoolInfo.idleThreadNum = threadNum;
		ret = pthread_mutex_init(&g_threadPoolInfo.lock, NULL);
		if (ret != 0) {
			dbg("pthread_mutex_init failed\n");
			break;
		}
		ret = sem_init(&g_threadPoolInfo.sem, 0, 0);
		if (ret != 0) {
			dbg("sem_init failed\n");
			pthread_mutex_destroy(&g_threadPoolInfo.lock);
			break;
		}
		int32_t idx;
		for (idx = 0; idx < threadNum; idx++) {
			ret = pthread_create(&g_threadPoolInfo.threadId[idx], NULL, PthreadPoolProcessThread, NULL);
			if (ret != 0) {
				dbg("create PthreadPoolProcessThread failed\n");
				ThreadPoolExitHandle(idx);
				break;
			}
		}
	} while (0);
	if (ret == 0) {
		dbg("ThreadPoolInit success\n");
		g_isInit = true;
	}
	pthread_mutex_unlock(&g_initMutex);

	return ret;
}

void ThreadPoolDeinit(void)
{
	dbg("enter ThreadPoolDeinit\n");
	pthread_mutex_lock(&g_initMutex);
	if (!g_isInit) {
		dbg("is not inited\n");
		pthread_mutex_unlock(&g_initMutex);
		return;
	}
	ThreadPoolExitHandle(g_threadPoolInfo.maxThreadNum);
	g_threadPoolInfo.curJobNum = 0;
	g_threadPoolInfo.maxThreadNum = 0;
	g_threadPoolInfo.idleThreadNum = 0;
	dbg("free list begin\n");
	DeInitDoublyLinkedListNode(&g_jobListHead);
	pthread_mutex_unlock(&g_initMutex);
}

int32_t ThreadPoolAddJob(ThreadPoolJobFunc jobFunc, void *arg)
{
	static int32_t i = 0;
	i++;
	dbg("enter ThreadPoolAddJob i: %d\n", i);
	if ((jobFunc == NULL) || (arg == NULL)) {
		dbg("invalid param\n");
		return -3;
	}
	
	pthread_mutex_lock(&g_initMutex);
	if (!g_isInit) {
		dbg("is not inited\n");
		pthread_mutex_unlock(&g_initMutex);
		return -1;
	}
	ThreadPoolJobNodeInfo *tmp = (ThreadPoolJobNodeInfo *)malloc(sizeof(ThreadPoolJobNodeInfo));
	if (tmp == NULL) {
		dbg("malloc failed\n");
		pthread_mutex_unlock(&g_initMutex);
		return -1;
	}
	tmp->jobInfo.jobFunc = jobFunc;
	tmp->jobInfo.arg = arg;
    DoublyLinkedListNode *node = CreateDoublyLinkedListNode(tmp);
    if (node == NULL) {
        dbg("CreateDoublyLinkedListNode failed\n");
        free(tmp);
        pthread_mutex_unlock(&g_initMutex);
		return -1;
    }
	pthread_mutex_lock(&g_threadPoolInfo.lock);
	dbg("threadPool status: curJobNum:%d, idleThreadNum: %d\n", g_threadPoolInfo.curJobNum, g_threadPoolInfo.idleThreadNum);
	(void)InsertHeadDoublyLinkedListNode(&g_jobListHead, node);
	g_threadPoolInfo.curJobNum++;
	pthread_mutex_unlock(&g_threadPoolInfo.lock);
	sem_post(&g_threadPoolInfo.sem);
	pthread_mutex_unlock(&g_initMutex);
	dbg("exit ThreadPoolAddJob\n");
	return 0;
}