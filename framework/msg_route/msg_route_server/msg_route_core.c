#include "msg_route_comm.h"
#include <stdint.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "thread_pool.h"
#include "my_log.h"

typedef struct {
    int32_t fd;
    bool msgFlag[MSG_ROUTE_MAX_MSG];
} MsgRouteClientInfo;

static MsgRouteClientInfo g_clientInfo[MAX_APP_ID];
static pthread_mutex_t g_clientInfoLock = PTHREAD_MUTEX_INITIALIZER;

static void MsgRouteClientInfoInit(void)
{
    (void)pthread_mutex_lock(&g_clientInfoLock);
    int32_t i;
    for (i = 0; i < MAX_APP_ID; i++) {
        g_clientInfo[i].fd = -1;
        memset(g_clientInfo[i].msgFlag, 0, sizeof(g_clientInfo[i].msgFlag));
    }
    (void)pthread_mutex_unlock(&g_clientInfoLock);
}

static void MsgRouteClientInfoDeinit(void)
{
    (void)pthread_mutex_lock(&g_clientInfoLock);
    int32_t i;
    for (i = 0; i < MAX_APP_ID; i++) {
        if (g_clientInfo[i].fd != -1) {
            (void)close(g_clientInfo[i].fd);
            g_clientInfo[i].fd = -1;
        }
        memset(g_clientInfo[i].msgFlag, 0, sizeof(g_clientInfo[i].msgFlag));
    }
    (void)pthread_mutex_unlock(&g_clientInfoLock);

}

int32_t MsgRouteCoreInit(int32_t threadNum)
{
    int32_t ret = ThreadPoolInit(threadNum);
    if (ret != 0) {
        dbg("ThreadPoolInit failed\n");
        return -1;
    }
    MsgRouteClientInfoInit();
    return 0;
}

void MsgRouteCoreDeinit(void)
{
    ThreadPoolDeinit();
    MsgRouteClientInfoDeinit();
}

static bool IsMsgRouteValidMsg(const MsgRouteDataInfo *msgData)
{
    if ((msgData->appId < ALL_APP_ID) || (msgData->appId >= MAX_APP_ID)) {
        dbg("invalid appId[%d]\n", msgData->appId);
        return false;
    }

    if ((msgData->msgType < MSG_ROUTE_BASE_MSG) || (msgData->msgType >= MSG_ROUTE_MAX_MSG)) {
        dbg("invalid msg type[%d]\n", msgData->msgType);
        return false;
    }
    return true;
}

static void MsgRouteCoreUpdateClientSubscribeInfo(const MsgRouteDataInfo *msgData, bool isAdd)
{
    (void)pthread_mutex_lock(&g_clientInfoLock);
    if (g_clientInfo[msgData->appId].fd == -1) {
        g_clientInfo[msgData->appId].fd = msgData->src;
    }
    g_clientInfo[msgData->appId].msgFlag[msgData->data[0]] = isAdd;
    (void)pthread_mutex_unlock(&g_clientInfoLock);
}

void MsgRouteCoreEraseClientInfo(int32_t fd)
{
    if (fd == -1) {
        return;
    }

    (void)pthread_mutex_lock(&g_clientInfoLock);
    int32_t i;
    for (i = 0; i < MAX_APP_ID; i++) {
        if (g_clientInfo[i].fd == fd) {
            g_clientInfo[i].fd = -1;
            memset(g_clientInfo[i].msgFlag, 0, sizeof(g_clientInfo[i].msgFlag));
            break;
        }
    }
    (void)pthread_mutex_unlock(&g_clientInfoLock);
}

static void MsgRouteCoreJobFunc(void *arg)
{
    if (arg == NULL) {
        return;
    }
    MsgRouteDataInfo *msgData = (MsgRouteDataInfo *)arg;
    int32_t dataLen = MSG_ROUTE_HEAD_SIZE + msgData->len;

    MsgRouteClientInfo clientInfo[MAX_APP_ID];
    (void)pthread_mutex_lock(&g_clientInfoLock);
    memcpy(clientInfo, g_clientInfo, sizeof(g_clientInfo));
    (void)pthread_mutex_unlock(&g_clientInfoLock);

    //这里小心自己把消息发送给自己，不过自己一般注册自己发送的消息类型
    int32_t ret;
    if (msgData->dataType == MSG_ROUTE_DATA_ANSWER_TYPE) {
        ret = send(msgData->src, arg, dataLen, 0);
        if (ret != dataLen) {
            dbg("send failed ret: %d, dataLen: %u\n", ret, dataLen);
        }
    } else if (msgData->appId != ALL_APP_ID) {
        if ((g_clientInfo[msgData->appId].fd != -1) && (g_clientInfo[msgData->appId].msgFlag[msgData->msgType])) {
            ret = send(g_clientInfo[msgData->appId].fd, arg, dataLen, 0);
            if (ret != dataLen) {
                dbg("send failed ret: %d, dataLen: %u\n", ret, dataLen);
            }
        } else {
            dbg("current app fd[%d] not subscribe msg this type: %d %d\n",
                g_clientInfo[msgData->appId].fd, msgData->msgType, g_clientInfo[msgData->appId].msgFlag[msgData->msgType]);
        }
    } else {
        int32_t i;
        for (i = 0; i < MAX_APP_ID; i++) {
            if ((g_clientInfo[i].fd != -1) && (g_clientInfo[i].msgFlag[msgData->msgType])) {
                ret = send(g_clientInfo[i].fd, arg, dataLen, 0);
                if (ret != dataLen) {
                    dbg("send failed ret: %d, dataLen: %u\n", ret, dataLen);
                }
            } else {
                dbg("current app fd[%d] not subscribe msg this type: %d %d\n",
                    g_clientInfo[i].fd, msgData->msgType, g_clientInfo[i].msgFlag[msgData->msgType]);
            }
        }
    }
    free(arg);
}

static void MsgRouteCoreAddJobToThreadPool(const MsgRouteDataInfo *msgData)
{
    uint32_t dataLen = MSG_ROUTE_HEAD_SIZE + msgData->len;
    uint8_t *data = (uint8_t *)malloc(dataLen);
    if (data == NULL) {
        dbg("malloc failed\n");
        return;
    }
    memcpy(data, msgData, dataLen);
    int32_t ret = ThreadPoolAddJob(MsgRouteCoreJobFunc, data);
    if (ret != 0) {
        dbg("ThreadPoolAddJob failed\n");
    }
}

void MsgRouteCoreHandleMsg(MsgRouteDataInfo *msgData)
{
    dbg("msgData->dataType: %d\n", msgData->dataType);
    dbg("msgData->msgType: %d\n", msgData->msgType);
    dbg("msgData->src: %d\n", msgData->src);
    dbg("msgData->appId: %d\n", msgData->appId);
    dbg("msgData->seq: %d\n", msgData->seq);
    dbg("msgData->timestamp: %lu\n", msgData->timestamp);
    dbg("msgData->len: %u\n", msgData->len);

    if (!IsMsgRouteValidMsg(msgData)) {
        return;
    }

    if (msgData->msgType == MSG_ROUTE_SUBSCRIBE_MSG) {
        MsgRouteCoreUpdateClientSubscribeInfo(msgData, true);
    } else if (msgData->msgType == MSG_ROUTE_UNSUBSCRIBE_MSG) {
        MsgRouteCoreUpdateClientSubscribeInfo(msgData, false);
    } else {
        MsgRouteCoreAddJobToThreadPool(msgData);
    }
}
