#include "msg_route_comm.h"
#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stddef.h>
#include <sys/epoll.h>
#include "msg_route_core.h"
#include "my_log.h"

#define MSG_ROUTE_EPOLL_FD_SIZE 100
static int32_t g_serverfd = -1;
static int32_t g_epollfd = -1;

static int32_t CreateMsgRouteServer(void)
{
    int32_t sock = socket(AF_UNIX, SOCK_SEQPACKET, 0);
    if (-1 == sock) {
        dbg("socket failed\n");
        return -1;
    }
    dbg("socket fd: %d\n", sock);

    struct sockaddr_un addr;
    (void)strcpy(addr.sun_path, MSG_ROUTE_SOCKET_FILE);
    (void)unlink(MSG_ROUTE_SOCKET_FILE);
    addr.sun_family = AF_UNIX;
    int32_t addrLen = offsetof(struct sockaddr_un, sun_path) + strlen(addr.sun_path);

    if (bind(sock, (struct sockaddr *)&addr, addrLen) != 0) {
        dbg("bind failed\n");
        (void)close(sock);
        return -1;
    }

    if (listen(sock, 5) != 0) {
        dbg("listen failed\n");
        (void)close(sock);
        return -1;
    }
    g_serverfd = sock;
    return 0;
}

static int32_t MsgEpollCtlEvent(int32_t epollFd, int32_t fd, int32_t event, int32_t ctlType)
{
    struct epoll_event ev;
    ev.events = event;
    ev.data.fd = fd;
    return epoll_ctl(epollFd, ctlType, fd, &ev);
}

static void MsgRouteServerAccept(int epollFd)
{
    int32_t clientFd = accept(g_serverfd, NULL, NULL);
    if (clientFd == -1) {
        dbg("accept failed\n");
        return;
    }

    int32_t ret = MsgEpollCtlEvent(epollFd, clientFd, EPOLLIN, EPOLL_CTL_ADD);
    if (ret != 0) {
        dbg("add clientFd[%d] epoll EPOLLIN failed\n", clientFd);
        return;
    }
    dbg("add clientFd[%d] epoll EPOLLIN success\n", clientFd);
}


static void MsgRouteServerHandleMsg(int32_t epollFd, int32_t clientFd, uint8_t *recvBuf, int32_t recvSize)
{
    int32_t recvByte = recv(clientFd, recvBuf, recvSize, 0);
    if (-1 == recvByte) {
        dbg("read clientFd[%d] error\n", clientFd);
        return;
    } else if (0 == recvByte) {
        dbg("clientFd[%d] disconnect\n", clientFd);
        MsgEpollCtlEvent(epollFd, clientFd, EPOLLIN, EPOLL_CTL_DEL);
        MsgRouteCoreEraseClientInfo(clientFd);
        (void)close(clientFd);
    } else {
        if ((recvByte > recvSize) || (recvByte <= MSG_ROUTE_HEAD_SIZE)) {
            dbg("recv error, recvByte[%d]\n", recvByte);
            return;
        }

        MsgRouteDataInfo *msgData = (MsgRouteDataInfo *)recvBuf;
        if ((recvByte - MSG_ROUTE_HEAD_SIZE) != msgData->len) {
            dbg("recv msg error, recvByte[%d], msgLen[%d]\n", recvByte, msgData->len);
            return;
        }
        msgData->src = clientFd; //!!!
        MsgRouteCoreHandleMsg(msgData);
    }
}

static void MsgRouteServerHandleEvent(struct epoll_event *events, int32_t num, uint8_t *recvBuf, int32_t recvSize)
{
    int32_t i;
    int32_t fd;
    for (i = 0; i < num; i++) {
        fd = events[i].data.fd;
        if ((fd == g_serverfd) && (events[i].events & EPOLLIN)) {
            MsgRouteServerAccept(g_epollfd);
        } else if (events[i].events & EPOLLIN) {
            MsgRouteServerHandleMsg(g_epollfd, fd, recvBuf, recvSize);
        } else {
            dbg("fd[%d] epoll_wait events is not EPOLLIN\n", fd);
        }
    }
}

static void *MsgRouteServerEpollThread(void *args)
{
    (void)pthread_detach(pthread_self());

    int32_t recvSize = MSG_ROUTE_HEAD_SIZE + MSG_ROUTE_DATA_SIZE;
    uint8_t *recvBuf = (uint8_t *)malloc(recvSize);
    if (recvBuf == NULL) {
        dbg("malloc failed\n");
        return NULL;
    }
    int32_t ret;
    struct epoll_event events[MSG_ROUTE_EPOLL_FD_SIZE];
    while (1) {
        errno = 0;
        ret = epoll_wait(g_epollfd, events, MSG_ROUTE_EPOLL_FD_SIZE, 0);
        if ((ret < 0) && (errno != EINTR)) {
            dbg("epoll_wait failed, %d, %s\n", errno, strerror(errno));
            break;
        } else if (ret < 0) {
            dbg("epoll_wait failed, %d, %s\n", errno, strerror(errno));
            continue;
        } else {
            MsgRouteServerHandleEvent(events, ret, recvBuf, recvSize);
        }
    }
    dbg("MsgRouteServerEpollThread exit\n");
    free(recvBuf);
    return NULL;
}

static void DeinitMsgRouteManager(void)
{
    if (g_serverfd != -1) {
        (void)close(g_serverfd);
        g_serverfd = -1;
    }

    if (g_epollfd != -1) {
        (void)close(g_epollfd);
        g_epollfd = -1;
    }
    MsgRouteCoreDeinit();
}

static int32_t InitMsgRouteManager(void)
{
    int32_t ret = CreateMsgRouteServer();
    if (ret != 0) {
        dbg("CreateMsgRouteServer failed\n");
        return -1;
    }

    do {
        ret = MsgRouteCoreInit(3);
        if (ret != 0) {
            dbg("epoll_create failed\n");
            break;
        }

        ret = epoll_create(MSG_ROUTE_EPOLL_FD_SIZE);
        if (ret < 0) {
            dbg("epoll_create failed\n");
            break;
        }

        g_epollfd = ret;
        ret = MsgEpollCtlEvent(g_epollfd, g_serverfd, EPOLLIN, EPOLL_CTL_ADD);
        if (ret != 0) {
            ret = -1;
            dbg("MsgEpollCtlEvent failed\n");
            break;
        }

        pthread_t tid;
        ret = pthread_create(&tid, NULL, MsgRouteServerEpollThread, NULL);
        if (ret != 0) {
            dbg("pthread_create failed\n");
            break;
        }
    } while (0);

    if (ret != 0) {
        DeinitMsgRouteManager();
    }

    return ret;
}

int32_t main(void)
{
    int32_t ret = InitMsgRouteManager();
    if (ret != 0) {
        dbg("InitMsgRouteManager failed\n");
        return -1;
    }

    while (1) {
        (void)sleep(10);
        dbg("manager is running\n");
    }
    return 0;
}
