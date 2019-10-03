#include "err_handler.h"
#include "logger.h"
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

#if !defined(WAPPERS_H)
#define WAPPERS_H

static int ret_wrapper = 0;

// IO api
int Close(int fd) {
    LOGGER_SIMP(LOG_LV_VERBOSE, "Wrapper: close");
    if ((ret_wrapper = close(fd)) == -1)
        errExitSimp("close failed");
    return ret_wrapper;
}

// socket api
int Socket(int domain, int type, int protocol) {
    LOGGER_SIMP(LOG_LV_VERBOSE, "Wrapper: socket");
    if ((ret_wrapper = socket(domain, type, protocol)) == -1)
        errExitSimp("create socket failed");
    return ret_wrapper;
}
int Bind(int fd, const sockaddr *addr, socklen_t len) {
    if ((ret_wrapper = bind(fd, addr, len)) == -1)
        errExitSimp("bind failed");
    return ret_wrapper;
}
int Listen(int fd, int backlog) {
    LOGGER_SIMP(LOG_LV_VERBOSE, "Wrapper: listen");
    if ((ret_wrapper = listen(fd, backlog)) == -1)
        errExitSimp("listen failed");
    return ret_wrapper;
}
int Accept(int sd, sockaddr *addr, socklen_t *len) {
    LOGGER_SIMP(LOG_LV_VERBOSE, "Wrapper: accept");
    if ((ret_wrapper = accept(sd, addr, len)) == -1)
        errExitSimp("accept failed");
    return ret_wrapper;
}

// epoll api
int Epoll_create(int sz) {
    LOGGER_SIMP(LOG_LV_VERBOSE, "Wrapper: epoll_create");
    ret_wrapper = epoll_create(sz);
    if (ret_wrapper == -1)
        errExitSimp("epoll create failed");
    return ret_wrapper;
}
int Epoll_ctl(int epfd, int op, int fd, epoll_event *events) {
    LOGGER_SIMP(LOG_LV_VERBOSE, "Wrapper: epoll_ctl");
    ret_wrapper = epoll_ctl(epfd, op, fd, events);
    if (ret_wrapper)
        errExitSimp("epoll ctl failed");
    return ret_wrapper;
}
int Epoll_wait(int epfd, epoll_event *evlist, int maxevents, int timeout) {
    LOGGER_SIMP(LOG_LV_VERBOSE, "Wrapper: epoll_wait");
    ret_wrapper = epoll_wait(epfd, evlist, maxevents, timeout);
    if (ret_wrapper == -1) {
        if (errno == EINTR)
            epoll_wait(epfd, evlist, maxevents, timeout);
        else
            errExitSimp("epoll wait failed");
    }
    return ret_wrapper;
}
void Epoll_add(int epfd, int fd, uint32_t events) {
    LOGGER_SIMP(LOG_LV_VERBOSE, "Wrapper: Epoll_add");
    epoll_event tmpev;
    tmpev.events = events;
    tmpev.data.fd = fd;
    Epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &tmpev);
}
void Epoll_del(int epfd, int fd, uint32_t events) {
    LOGGER_SIMP(LOG_LV_VERBOSE, "Wrapper: Epoll_del");
    epoll_event tmpev;
    tmpev.events = events;
    tmpev.data.fd = fd;
    Epoll_ctl(epfd, EPOLL_CTL_DEL, fd, &tmpev);
}

#endif // WAPPERS_H
