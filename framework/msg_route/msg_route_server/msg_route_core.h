#ifndef MSG_ROUTE_CORE_H
#define MSG_ROUTE_CORE_H

#include <stdint.h>
#include "msg_route_comm.h"

int32_t MsgRouteCoreInit(int32_t threadNum);
void MsgRouteCoreDeinit(void);
void MsgRouteCoreHandleMsg(MsgRouteDataInfo *msgData);
void MsgRouteCoreEraseClientInfo(int32_t fd);

#endif
