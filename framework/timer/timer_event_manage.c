#include "timer_event_manage.h"
#include <stdbool.h>
#include <pthread.h>
#include <errno.h>
#include <sys/timerfd.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <string.h>
#include "my_log.h"

typedef struct {
    int32_t timerFd;
    TimerEventProcCb cb;
} TimerEventManageInfo;

static bool g_isInit = false;
static pthread_mutex_t g_initMutex = PTHREAD_MUTEX_INITIALIZER;

static TimerEventManageInfo g_eventInfo[TIMER_EVENT_NUM_MAX + 1];
static pthread_mutex_t g_eventInfoMutex = PTHREAD_MUTEX_INITIALIZER;

static bool g_isRunning = false;
static int32_t g_maxEventNum = 0;
static int32_t g_epollfd = -1;
static pthread_t g_tid;

static int32_t TimerEventManageHeartbeatCb(int32_t timerFd, uint64_t count)
{
    dbg("TimerEventManageHeartbeatCb timerFd: %d, count: %lu\n", timerFd, count);
    return 0;
}

static void TimerEventManageHandleEvent(struct epoll_event *events, int32_t num)
{
    TimerEventManageInfo eventInfo[TIMER_EVENT_NUM_MAX + 1];
    pthread_mutex_lock(&g_eventInfoMutex);
    (void)memcpy(eventInfo, g_eventInfo, sizeof(g_eventInfo));
    pthread_mutex_unlock(&g_eventInfoMutex);

    uint64_t count = 0;
    int32_t i, j;
    int32_t fd;
    for (i = 0; i < num; i++) {
        fd = events[i].data.fd;
        (void)read(fd, &count, sizeof(count));
        for (j = 0; j < g_maxEventNum; j++) {
            if (fd == eventInfo[j].timerFd) {
                if (eventInfo[j].cb != NULL) {
                    eventInfo[j].cb(fd, count);
                }
                break;
            }
        }
    }
}

static void *TimerEventManageHandleThread(void *arg)
{
    int32_t ret;
    struct epoll_event events[TIMER_EVENT_NUM_MAX + 1];

    while (g_isRunning) {
        errno = 0;
        ret = epoll_wait(g_epollfd, events, TIMER_EVENT_NUM_MAX + 1, 0);
        if ((ret < 0) && (errno != EINTR)) {
            dbg("epoll_wait failed, %d, %s\n", errno, strerror(errno));
            break;
        } else if (ret < 0) {
            dbg("epoll_wait failed, %d, %s\n", errno, strerror(errno));
            continue;
        } else {
            TimerEventManageHandleEvent(events, ret);
        }
    }
    dbg("TimerEventManageHandleThread exit\n");
}

static int32_t TimerEventEpollCtlEvent(int32_t epollFd, int32_t fd, int32_t event, int32_t ctlType)
{
    struct epoll_event ev;
    ev.events = event;
    ev.data.fd = fd;
    return epoll_ctl(epollFd, ctlType, fd, &ev);
}

static int32_t DeinitTimerEventManageCore(void)
{
    if (g_eventInfo[0].timerFd != -1) {
        (void)TimerEventEpollCtlEvent(g_epollfd, g_eventInfo[0].timerFd, EPOLLIN, EPOLL_CTL_DEL);
        (void)close(g_eventInfo[0].timerFd);
        g_eventInfo[0].timerFd = -1;
        g_eventInfo[0].cb = NULL;
    }
    if (g_epollfd != -1) {
        (void)close(g_epollfd);
        g_epollfd = -1;
    }
    g_maxEventNum = 0;
    g_isInit = false;
}

static void InitTimerEventManageData(void)
{
    for (int32_t i = 0; i < (TIMER_EVENT_NUM_MAX + 1); i++) {
        g_eventInfo[i].cb = NULL;
        g_eventInfo[i].timerFd = -1;
    }
    g_isRunning = true;
    g_maxEventNum = 0;
}


int32_t InitTimerEventManage(int32_t maxEventNum)
{
    pthread_mutex_lock(&g_initMutex);
    if (g_isInit) {
        dbg("is inited\n");
        pthread_mutex_unlock(&g_initMutex);
        return 0;
    }

    int32_t ret = -1;
    InitTimerEventManageData();
    do {
        if ((maxEventNum <= 0) || (maxEventNum > TIMER_EVENT_NUM_MAX)) {
            dbg("invalid maxEventNum: %d\n", maxEventNum);
            break;
        }

        g_maxEventNum = maxEventNum + 1;
        g_epollfd = epoll_create(g_maxEventNum);
        if (g_epollfd < 0) {
            dbg("epoll_create failed\n");
            break;
        }

        g_eventInfo[0].cb = TimerEventManageHeartbeatCb;
        g_eventInfo[0].timerFd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK);
        if (g_eventInfo[0].timerFd == -1) {
            dbg("timerfd_create failed\n");
            break;
        }

        ret = TimerEventEpollCtlEvent(g_epollfd, g_eventInfo[0].timerFd, EPOLLIN, EPOLL_CTL_ADD);
        if (ret != 0) {
            dbg("TimerEventEpollCtlEvent failed\n");
            break;
        }

        ret = pthread_create(&g_tid, NULL, TimerEventManageHandleThread, NULL);
        if (ret != 0) {
            dbg("timerfd_create failed\n");
            break;
        }
    } while(0);
    if (ret != 0) {
        DeinitTimerEventManageCore();
    } else {
        g_isInit = true;
        SetTimerEvent(g_eventInfo[0].timerFd, 1000, 5000);
    }
    pthread_mutex_unlock(&g_initMutex);
    return ret;
}

void DeinitTimerEventManage(void)
{
    pthread_mutex_lock(&g_initMutex);
    if (!g_isInit) {
        dbg("isn't inited\n");
        pthread_mutex_unlock(&g_initMutex);
        return;
    }
    g_isRunning = false;
    SetTimerEvent(g_eventInfo[0].timerFd, 100, 100);
    pthread_join(g_tid, NULL);
    dbg("pthread_join finished\n");
    pthread_mutex_lock(&g_eventInfoMutex);
    for (int32_t i = 0; i < g_maxEventNum; i++) {
        g_eventInfo[i].cb = NULL;
        if (g_eventInfo[i].timerFd != -1) {
            (void)TimerEventEpollCtlEvent(g_epollfd, g_eventInfo[i].timerFd, EPOLLIN, EPOLL_CTL_DEL);
            (void)close(g_eventInfo[i].timerFd);
            g_eventInfo[i].timerFd = -1;
        }
    }
    DeinitTimerEventManageCore();
    pthread_mutex_unlock(&g_eventInfoMutex);
    pthread_mutex_unlock(&g_initMutex);
}

int32_t AddTimerEvent(TimerEventProcCb cb)
{
    if (cb == NULL) {
        dbg("cb is null\n");
        return -1;
    }
    pthread_mutex_lock(&g_initMutex);
    if (!g_isInit) {
        dbg("isn't inited\n");
        pthread_mutex_unlock(&g_initMutex);
        return -1;
    }
    pthread_mutex_unlock(&g_initMutex);

    pthread_mutex_lock(&g_eventInfoMutex);
    int32_t timerFd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK);
    if (timerFd == -1) {
        dbg("timerfd_create failed\n");
        pthread_mutex_unlock(&g_eventInfoMutex);
        return -1;
    }
    int32_t i;
    for (i = 0; i < g_maxEventNum; i++) {
        if (g_eventInfo[i].timerFd == -1) {
            (void)TimerEventEpollCtlEvent(g_epollfd, timerFd, EPOLLIN, EPOLL_CTL_ADD);
            g_eventInfo[i].timerFd = timerFd;
            g_eventInfo[i].cb = cb;
            break;
        }
    }
    pthread_mutex_unlock(&g_eventInfoMutex);
    if (i == g_maxEventNum) {
        dbg("event is full, AddTimerEvent failed\n");
        close(timerFd);
        return -1;
    }
    dbg("AddTimerEvent success\n");
    return timerFd;
}
int32_t DeleteTimerEvent(int32_t timerFd)
{
    if (timerFd < 0) {
        dbg("invalid timerFd: %d\n", timerFd);
        return -1;
    }
    pthread_mutex_lock(&g_initMutex);
    if (!g_isInit) {
        dbg("isn't inited\n");
        pthread_mutex_unlock(&g_initMutex);
        return -1;
    }
    pthread_mutex_unlock(&g_initMutex);

    pthread_mutex_lock(&g_eventInfoMutex);
    int32_t i;
    for (i = 0; i < g_maxEventNum; i++) {
        if (g_eventInfo[i].timerFd == timerFd) {
            (void)TimerEventEpollCtlEvent(g_epollfd, timerFd, EPOLLIN, EPOLL_CTL_DEL);
            close(timerFd);
            g_eventInfo[i].timerFd = -1;
            g_eventInfo[i].cb = NULL;
            dbg("find delete timerFd\n");
            break;
        }
    }
    pthread_mutex_unlock(&g_eventInfoMutex);
    dbg("DeleteTimerEvent success\n");
    return 0;

}

int32_t SetTimerEvent(int32_t timerFd, uint16_t startTime, uint16_t period)
{
    if (timerFd < 0) {
        dbg("invalid timerFd: %d\n", timerFd);
        return -1;
    }
    struct itimerspec time;

    time.it_value.tv_sec = startTime / 1000;
    time.it_value.tv_nsec = (startTime % 1000) * 1000 * 1000;
    // 设置超时间隔
    time.it_interval.tv_sec = period / 1000;
    time.it_interval.tv_nsec = (period % 1000) * 1000 * 1000;;
    return timerfd_settime(timerFd, CLOCK_MONOTONIC, &time, NULL);
}

