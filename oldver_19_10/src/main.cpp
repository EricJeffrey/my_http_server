/**
 * Http服务器，提供静态文件映射与动态脚本服务功能
 * EricJeffrey 2019/10/3
*/

#include "thread_pool.h"
#include "wrappers.h"

#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/stat.h>

int num_tot_thread = 4;
int back_log = 20;
unsigned int port = 2333;
int time_out_epoll = 100;
int max_events_epoll = 10;

int openListenfd(int port, int back_log);
void startEpollWait(int epfd, int sd, int time_out_epoll, int max_events_epoll);
int acceptClient(int epfd, int sd);
void handleRequest(int fd);
void handleResponse(int fd);
void init();

int main(int argc, char const *argv[]) {
    init();

    int sd = openListenfd(port, back_log);

    int epfd = Epoll_create(1);
    Epoll_add(epfd, sd, EPOLLIN);

    LOGGER_SIMP(LOG_LV_DEBUG, "starting epoll wait");

    startEpollWait(epfd, sd, time_out_epoll, max_events_epoll);

    return 0;
}

// 初始化配置信息
void init() {
    freopen("server.log", "w", stderr);
    setbuf(stderr, NULL);
    LOGGER_SIMP(LOG_LV_DEBUG, "init");

    LOGGER_SET_LV(LOG_LV_VERBOSE);
    thread_pool::getInstance(num_tot_thread);
}

// 打开监听端口，返回套接字描述符
int openListenfd(int port, int back_log) {
    LOGGER_SIMP(LOG_LV_VERBOSE, "openListenfd");

    int sd = 0;
    sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    sd = Socket(AF_INET, SOCK_STREAM, 0);
    Bind(sd, (sockaddr *)&addr, sizeof(addr));
    Listen(sd, back_log);

    LOGGER_FORMAT(LOG_LV_DEBUG, "listening port %d", port);
    return sd;
}

// 接收客户端的连接请求，返回创建的套接字描述符
int acceptClient(int epfd, int sd) {
    LOGGER_SIMP(LOG_LV_VERBOSE, "acceptClient");

    int tmpfd;
    sockaddr_in tmpaddr;
    socklen_t len = sizeof(tmpaddr);
    tmpfd = Accept(sd, (sockaddr *)&tmpaddr, &len);
    Epoll_add(epfd, tmpfd, EPOLLIN);
    // Fcntl_NBlock(tmpfd);
    return tmpfd;
}

// 处理客户端的HTTP请求
void handleRequest(int fd) {
    LOGGER_SIMP(LOG_LV_VERBOSE, "handleRequest");
    // 提交任务到线程池 TODO
    runnable runner = [](task_arg arg) {
        int fd = arg.fd;
        LOGGER_FORMAT(LOG_LV_VERBOSE, "runner fd = %d: start", fd);
        const int buf_size = 1024;
        char buf[buf_size];
        int n = read(fd, buf, buf_size);
        LOGGER_FORMAT(LOG_LV_VERBOSE, "runner fd = %d: read over", fd);
        LOGGER_FORMAT(LOG_LV_DEBUG, "runner fd = %d: read data: %s", fd, buf);

        char bufout[buf_size << 1] = {};
        int len = strlen(buf) + 61;
        n = snprintf(bufout, buf_size << 1, "HTTP1.0 %d OK\r\ncontent-language: en-US\r\ncontent-type: text/html; charset=utf-8\r\ncontent-length: %d\r\n\r\n%s\n<strong style=\"font-size:72px;\"><br/>Hello World!<br/><strong>", 200, len, buf);
        n = write(fd, bufout, n);
        LOGGER_FORMAT(LOG_LV_VERBOSE, "runner fd = %d: write over", fd);

        // Close(fd);
    };

    thread_pool::getInstance(num_tot_thread).pushTask(task(runner, task_arg(fd)));
    LOGGER_FORMAT(LOG_LV_DEBUG, "task pushed into thread pool, task fd: %d", fd);
}

// 开始监听文件描述符上的事件
void startEpollWait(int epfd, int sd, int time_out_epoll, int max_events_epoll) {
    LOGGER_SIMP(LOG_LV_VERBOSE, "startEpollWait");

    while (true) {
        int num_ready = 0;
        struct epoll_event evlist[max_events_epoll];
        num_ready = Epoll_wait(epfd, evlist, max_events_epoll, time_out_epoll);
        LOGGER_FORMAT(LOG_LV_DEBUG, "epoll_wait return, ready clients num: %d", num_ready);

        for (int i = 0; i < num_ready; i++) {
            LOGGER_FORMAT(LOG_LV_DEBUG, "fd=%d; events: %s%s%s%s, event_value: %d", evlist[i].data.fd,
                          (evlist[i].events & EPOLLIN) ? "EPOLLIN " : "",
                          (evlist[i].events & EPOLLHUP) ? "EPOLLHUP " : "",
                          (evlist[i].events & EPOLLOUT) ? "EPOLLOUT " : "",
                          (evlist[i].events & EPOLLERR) ? "EPOLLERR " : "",
                          evlist[i].events);

            epoll_event tmpev = evlist[i];

            if (tmpev.events & (EPOLLRDHUP | EPOLLHUP)) { // 流套接字对端关闭连接
                Close(tmpev.data.fd);
                Epoll_del(epfd, tmpev.data.fd, EPOLLIN);
                LOGGER_FORMAT(LOG_LV_DEBUG, "epoll: RD HUP, fd = %d deleted", tmpev.data.fd);
            } else if (tmpev.events & EPOLLIN) { // 客户端连接或数据读取
                if (tmpev.data.fd == sd) {       // 客户端建立连接
                    LOGGER_SIMP(LOG_LV_DEBUG, "epoll: new client");
                    acceptClient(epfd, sd);
                } else { // 客户端数据传输请求
                    LOGGER_SIMP(LOG_LV_DEBUG, "epoll: new data");
                    handleRequest(tmpev.data.fd);
                }
            } else if (tmpev.events & EPOLLOUT) {
                LOGGER_SIMP(LOG_LV_DEBUG, "epoll: data ready out");
            } else if (tmpev.events & EPOLLERR) { //错误
                LOGGER_SIMP(LOG_LV_DEBUG, "epoll_wait error!");
                errExitSimp("epoll failed");
            }
        }
        // 睡一会，方便Debug
        useconds_t tmptime = 200;
        LOGGER_FORMAT(LOG_LV_DEBUG, "sleep for %d seconds, %d seconds left.", tmptime, usleep(tmptime));
    }
}
