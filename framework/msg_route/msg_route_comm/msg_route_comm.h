#ifndef MSG_ROUTE_COMM_H
#define MSG_ROUTE_COMM_H

#include <stdint.h>

#define MSG_ROUTE_SOCKET_FILE "/home/yxw/socket/msg_route.socket"
#define MSG_ROUTE_HEAD_SIZE sizeof(MsgRouteDataInfo)
#define MSG_ROUTE_DATA_SIZE 4096

typedef enum {
    ALL_APP_ID,
    V2X_APP_ID,
    GNSS_APP_ID,
    MAX_APP_ID,
} MsgRouteClientId;

typedef enum {
    MSG_ROUTE_DATA_NOTIFY_TYPE,
    MSG_ROUTE_DATA_NEED_RESP_TYPE,
    MSG_ROUTE_DATA_ANSWER_TYPE,
} MsgRouteDataType;

typedef struct {
    int32_t dataType; //MsgRouteDataType
    int32_t msgType; // MsgRouteMsgType
    int32_t src;
    int32_t appId;
    uint64_t timestamp;
    int32_t seq;
    uint32_t len; //
    uint8_t data[0];
} MsgRouteDataInfo;

typedef enum {
    MSG_ROUTE_BASE_MSG,
    MSG_ROUTE_SUBSCRIBE_MSG,
    MSG_ROUTE_UNSUBSCRIBE_MSG,
    MSG_ROUTE_V2X_HV_MSG,
    MSG_ROUTE_V2X_RV_MSG,
    MSG_ROUTE_V2X_TA_MSG,
    MSG_ROUTE_GNSS_MSG,
    MSG_ROUTE_MAX_MSG,
} MsgRouteMsgType;

#endif