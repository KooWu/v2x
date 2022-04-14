#ifndef TIMER_EVENT_MANAGE_H
#define TIMER_EVENT_MANAGE_H

#include <stdint.h>

#define TIMER_EVENT_NUM_MAX 10
typedef int32_t (*TimerEventProcCb)(int32_t timerFd, uint64_t count);

int32_t InitTimerEventManage(int32_t maxEventNum);
void DeinitTimerEventManage(void);
int32_t AddTimerEvent(TimerEventProcCb cb);
int32_t DeleteTimerEvent(int32_t timerFd);
int32_t SetTimerEvent(int32_t timerFd, uint16_t startTime, uint16_t period);

#endif