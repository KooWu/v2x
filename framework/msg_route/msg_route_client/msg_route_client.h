#ifndef MSG_ROUTE_SERVER
#define MSG_ROUTE_SERVER

#include <stdint.h>
#include <stdbool.h>
#include "msg_route_comm.h"

typedef int32_t (*MsgRouteClientCallback)(MsgRouteDataInfo *msgData);

typedef struct {
    int32_t msgType;
    MsgRouteClientCallback rxFunc;
} MsgRouteClientSubscribeInfo;

int32_t MsgRouteClientInit(int32_t appId);
void MsgRouteClientDeinit(void);
int32_t MsgRouteClientSendMsg(MsgRouteDataInfo *sendData, bool isBroadcast);
int32_t MsgRouteClientSubscribeMsg(MsgRouteClientSubscribeInfo *subscribeInfo);
int32_t MsgRouteClientUnsubscribeMsg(MsgRouteClientSubscribeInfo *subscribeInfo);

#endif