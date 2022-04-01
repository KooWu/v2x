#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "msg_route_comm.h"
#include "msg_route_client.h"

static int32_t TestMsgRouteClientCallback(MsgRouteDataInfo *msgData)
{
    printf("msgData->dataType: %d\n", msgData->dataType);
    printf("msgData->msgType: %d\n", msgData->msgType);
    printf("msgData->src: %d\n", msgData->src);
    printf("msgData->appId: %d\n", msgData->appId);
    printf("msgData->seq: %d\n", msgData->seq);
    printf("msgData->timestamp: %lu\n", msgData->timestamp);
    printf("msgData->len: %u\n", msgData->len);
    printf("msgData->data: %s\n", msgData->data);
    return 0;
}

int main(int argc, char **argv)
{
    int32_t appId = V2X_APP_ID;
    if (argc > 1) {
        appId = atoi(argv[1]);
    }
    printf("appId: %d\n", appId);
    int32_t ret = MsgRouteClientInit(appId);
    if (ret != 0) {
        printf("MsgRouteClientInit failed\n");
        return -1;
    }

    MsgRouteClientSubscribeInfo subscribeInfo;
    if (appId == V2X_APP_ID) {
        subscribeInfo.msgType = MSG_ROUTE_GNSS_MSG;
        subscribeInfo.rxFunc = TestMsgRouteClientCallback;
        ret = MsgRouteClientSubscribeMsg(&subscribeInfo);
        if (ret != 0) {
            printf("MsgRouteClientSubscribeMsg failed\n");
            return -1;
        }
#if 1
        subscribeInfo.msgType = MSG_ROUTE_V2X_HV_MSG;
        subscribeInfo.rxFunc = TestMsgRouteClientCallback;
        ret = MsgRouteClientSubscribeMsg(&subscribeInfo);
        if (ret != 0) {
            printf("MsgRouteClientSubscribeMsg failed\n");
            return -1;
        }
#endif
    } else {
        subscribeInfo.msgType = MSG_ROUTE_V2X_HV_MSG;
        subscribeInfo.rxFunc = TestMsgRouteClientCallback;
        ret = MsgRouteClientSubscribeMsg(&subscribeInfo);
        if (ret != 0) {
            printf("MsgRouteClientSubscribeMsg failed\n");
            return -1;
        }
#if 1
        subscribeInfo.msgType = MSG_ROUTE_GNSS_MSG;
        subscribeInfo.rxFunc = TestMsgRouteClientCallback;
        ret = MsgRouteClientSubscribeMsg(&subscribeInfo);
        if (ret != 0) {
            printf("MsgRouteClientSubscribeMsg failed\n");
            return -1;
        }
#endif
    }

    uint8_t buf[512];
    MsgRouteDataInfo *sendData = (MsgRouteDataInfo *)buf;
    sendData->dataType = MSG_ROUTE_DATA_NOTIFY_TYPE;
    
    int32_t sendAppId;
    int32_t i = 0;
    while (1) {
        printf("input send app id:\n");
        scanf("%d", &sendAppId);
        printf("appId: %d, sendAppId : %d\n", appId, sendAppId);
        if (sendAppId < ALL_APP_ID || sendAppId >= MAX_APP_ID) {
            break;
        }
        if (sendAppId == V2X_APP_ID) {
            sendData->msgType = MSG_ROUTE_GNSS_MSG;
        } else if (sendAppId == GNSS_APP_ID) {
            sendData->msgType = MSG_ROUTE_V2X_HV_MSG;
        } else {
            sendData->msgType = MSG_ROUTE_V2X_HV_MSG;
        }
        int32_t len = sprintf(buf + MSG_ROUTE_HEAD_SIZE, "%02d hello %02d: %05d", appId, sendAppId, i);
        sendData->appId = sendAppId;
        sendData->len = len;
        ret = MsgRouteClientSendMsg(sendData, false);
        printf("MsgRouteClientSendMsg ret: %d\n", ret);
        i++;
    }
    return 0;
}