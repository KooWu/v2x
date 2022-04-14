#include "timer_event_manage.h"
#include <stdio.h>
#include <stdint.h>

static int32_t TimerEventManageTestCb(int32_t timerFd, uint64_t count)
{
    printf("TimerEventManageHeartbeatCb timerFd: %d, count: %lu\n", timerFd, count);
    return 0;
}

int32_t EnterTestTimerEventManage(void)
{
	int32_t ret = InitTimerEventManage(3);
    if (ret != 0) {
        printf("InitTimerEventManage failed\n");
        return -1;
    }

    int32_t timerid[10];
    for (int32_t i = 0; i < 5; i++) {
        timerid[i] = AddTimerEvent(TimerEventManageTestCb);
        if (timerid[i] < 0) {
            printf("AddTimerEvent failed\n");
            break;
        }
    }

    DeleteTimerEvent(timerid[0]);
    timerid[0] = AddTimerEvent(TimerEventManageTestCb);
    SetTimerEvent(timerid[0], 100, 1000);

    sleep(4);
    printf("close\n");
    SetTimerEvent(timerid[0], 0, 0);

    sleep(3);
    printf("start one time\n");
    SetTimerEvent(timerid[0], 100, 0);
    int32_t isExit;
    while (1) {
        scanf("%d", &isExit);
		if (isExit == 0) {
			break;
		}
    }
    DeinitTimerEventManage();
    return 0;
}

int main(void)
{
	printf("test timer event manage begin\n");
	EnterTestTimerEventManage();
    //printf("test timer event manage end\n");
	return 0;
}

