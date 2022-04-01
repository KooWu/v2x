#include "msg_route_client.h"
#include <stdint.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stddef.h>
#include "msg_route_comm.h"
#include "my_log.h"

static MsgRouteClientSubscribeInfo g_subscribeInfo[MSG_ROUTE_MAX_MSG];
static pthread_mutex_t g_subscribeInfoLock = PTHREAD_MUTEX_INITIALIZER;
static bool g_isInit = false;
static pthread_mutex_t g_initLock = PTHREAD_MUTEX_INITIALIZER;
static int32_t g_fd = -1;
static bool g_isRunning = false;
static int32_t g_curAppId = ALL_APP_ID;

static int32_t MsgRouteClientCreate(void)
{
    int32_t fd = socket(AF_UNIX, SOCK_SEQPACKET, 0);
    if (fd == -1) {
        dbg("socket failed\n");
        return -1;
    }

    struct sockaddr_un addr;
    (void)strcpy(addr.sun_path, MSG_ROUTE_SOCKET_FILE);
    addr.sun_family = AF_UNIX;
    int32_t addrLen = offsetof(struct sockaddr_un, sun_path) + strlen(addr.sun_path);

    if (connect(fd, (struct sockaddr *)&addr, addrLen) < 0) {
        dbg("connect failed\n");
        (void)close(fd);
        return -1;
    }
    g_fd = fd;
    return 0;
}

static void *MsgRouteClientRxThread(void *arg)
{
    (void)pthread_detach(pthread_self());

    int32_t recvSize = MSG_ROUTE_HEAD_SIZE + MSG_ROUTE_DATA_SIZE;
    uint8_t *recvBuf = (uint8_t *)malloc(recvSize);
    if (recvBuf == NULL) {
        dbg("malloc failed\n");
        return NULL;
    }
    int32_t recvByte;
    while (g_isRunning) {
        recvByte = recv(g_fd, recvBuf, recvSize, 0);
        if (-1 == recvByte) {
            dbg("read error\n");
            continue;
        } else if (0 == recvByte) {
            dbg("server disconnect\n");
            (void)close(g_fd);
            g_fd = -1;
            break;
        } else {
            if ((recvByte > recvSize) || (recvByte <= MSG_ROUTE_HEAD_SIZE)) {
                dbg("recv error, recvByte[%d]\n", recvByte);
                continue;
            }
            MsgRouteDataInfo *msgData = (MsgRouteDataInfo *)recvBuf;
            if ((msgData->msgType < MSG_ROUTE_BASE_MSG) || (msgData->msgType >= MSG_ROUTE_MAX_MSG)) {
                dbg("invalid msg type[%d]\n", msgData->msgType);
                continue;
            }
            pthread_mutex_lock(&g_subscribeInfoLock);
            MsgRouteClientSubscribeInfo subscribeInfo = g_subscribeInfo[msgData->msgType];
            pthread_mutex_unlock(&g_subscribeInfoLock);
            if ((subscribeInfo.rxFunc != NULL) && (subscribeInfo.msgType == msgData->msgType)) {
                subscribeInfo.rxFunc(msgData);
            }
        }
    }
    free(recvBuf);
    dbg("MsgRouteClientRxThread exit\n");
    return NULL;
}

int32_t MsgRouteClientInit(int32_t appId)
{
    if ((appId <= ALL_APP_ID) || (appId >= MAX_APP_ID)) {
        dbg("invalid app id: %d\n", appId);
        return -1;
    }
    pthread_mutex_lock(&g_initLock);
    if (g_isInit) {
        dbg("is inited\n");
        pthread_mutex_unlock(&g_initLock);
        return 0;
    }

    int32_t ret;
    do {
        ret = MsgRouteClientCreate();
        if (ret != 0) {
            dbg("MsgRouteClientCreate failed\n");
            break;
        }

        g_isRunning = true;
        pthread_t tid;
        ret = pthread_create(&tid, NULL, MsgRouteClientRxThread, NULL);
        if (ret != 0) {
            dbg("pthread_create failed\n");
            break;
        }
        g_curAppId = appId;
    } while (0);
    pthread_mutex_unlock(&g_initLock);

    return ret;
}

void MsgRouteClientDeinit(void)
{
    pthread_mutex_lock(&g_initLock);
    g_isRunning = false;
    (void)close(g_fd);
    g_fd = -1;
    sleep(2);
    g_isInit = false;
    g_curAppId = ALL_APP_ID;
    pthread_mutex_unlock(&g_initLock);
}

static int32_t MsgRouteClientSendCommonMsg(MsgRouteDataInfo *sendData)
{
    sendData->src = g_fd;
    sendData->timestamp = 0;
    sendData->seq = 0;
    int32_t ret = send(g_fd, sendData, MSG_ROUTE_HEAD_SIZE + sendData->len, 0);
    if (ret < 0) {
        dbg("send failed, ret: %d\n", ret);
        return -1;
    }
    return 0;
}

int32_t MsgRouteClientSendMsg(MsgRouteDataInfo *sendData, bool isBroadcast)
{
    if (sendData == NULL) {
        dbg("invalid sendData\n");
        return -1;
    }

    if (isBroadcast) {
        sendData->appId = ALL_APP_ID;
    }
    return MsgRouteClientSendCommonMsg(sendData);
}

int32_t MsgRouteClientSubscribeMsg(MsgRouteClientSubscribeInfo *subscribeInfo)
{
    if (subscribeInfo == NULL) {
        dbg("invalid param");
        return -1;
    }

    if ((subscribeInfo->msgType < MSG_ROUTE_BASE_MSG) || (subscribeInfo->msgType >= MSG_ROUTE_MAX_MSG)) {
        dbg("invalid msg type[%d]\n", subscribeInfo->msgType);
        return -1;
    }

    if (subscribeInfo->rxFunc == NULL) {
        dbg("rxFunc is NULL\n");
        return -1;
    }

    uint8_t sendBuf[512];
    MsgRouteDataInfo *sendData = (MsgRouteDataInfo *)sendBuf;
    sendData->dataType = MSG_ROUTE_DATA_NOTIFY_TYPE;
    sendData->msgType = MSG_ROUTE_SUBSCRIBE_MSG;
    sendData->appId = g_curAppId;
    sendData->len = 1;
    sendData->data[0] = (uint8_t)subscribeInfo->msgType;
    int32_t ret = MsgRouteClientSendCommonMsg(sendData);
    if (ret != 0) {
        dbg("MsgRouteClientSendCommonMsg failed\n");
        return -1;
    }
    pthread_mutex_lock(&g_subscribeInfoLock);
    g_subscribeInfo[subscribeInfo->msgType].msgType = subscribeInfo->msgType;
    g_subscribeInfo[subscribeInfo->msgType].rxFunc = subscribeInfo->rxFunc;
    pthread_mutex_unlock(&g_subscribeInfoLock);
    return 0;
}

int32_t MsgRouteClientUnsubscribeMsg(MsgRouteClientSubscribeInfo *subscribeInfo)
{
    if (subscribeInfo == NULL) {
        dbg("invalid param");
        return -1;
    }

    if ((subscribeInfo->msgType < MSG_ROUTE_BASE_MSG) || (subscribeInfo->msgType >= MSG_ROUTE_MAX_MSG)) {
        dbg("invalid msg type[%d]\n", subscribeInfo->msgType);
        return -1;
    }

    uint8_t sendBuf[512];
    MsgRouteDataInfo *sendData = (MsgRouteDataInfo *)sendBuf;
    sendData->dataType = MSG_ROUTE_DATA_NOTIFY_TYPE;
    sendData->msgType = MSG_ROUTE_UNSUBSCRIBE_MSG;
    sendData->appId = g_curAppId;
    sendData->len = 1;
    sendData->data[0] = (uint8_t)subscribeInfo->msgType;
    int32_t ret = MsgRouteClientSendCommonMsg(sendData);
    if (ret != 0) {
        dbg("MsgRouteClientSendCommonMsg failed\n");
        return -1;
    }
    pthread_mutex_lock(&g_subscribeInfoLock);
    g_subscribeInfo[subscribeInfo->msgType].msgType = MSG_ROUTE_BASE_MSG;
    g_subscribeInfo[subscribeInfo->msgType].rxFunc = NULL;
    pthread_mutex_unlock(&g_subscribeInfoLock);
    return 0;
}