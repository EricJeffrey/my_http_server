/**
 * Http服务器，提供静态文件映射与动态脚本服务功能
 * EricJeffrey 2019/10/3
*/

#include "thread_pool.h"
#include "wrappers.h"

#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <sys/fcntl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

int num_tot_thread = 6;

int openListenfd(int port, int back_log);
void startEpollWait(int epfd, int sd, int time_out_epoll, int max_events_epoll);
int acceptClient(int epfd, int sd);
void handleRequest(int fd);

// 测试线程池是否能正常工作
void work() {
    runnable runner = [](void *arg) {
        int t = *(int *)arg;
        fprintf(stderr, "run now, sleep %d seconds...\n", t);
        sleep(t);
        fprintf(stderr, "run over...\n");
    };
    thread_pool &pool = thread_pool::getInstance(3);
    int sleeptimes[] = {3, 2, 2, 4, 1, 3, 2, 4, 1, 2, 2, 3, 2, 4, 1, 2};
    for (int i = 0; i < 16; i++) {
        int *t = (int *)malloc(sizeof(int));
        *t = sleeptimes[i];
        pool.pushTask(task(runner, t));
    }
    getchar();
    exit(0);
}

int main(int argc, char const *argv[]) {
    LOGGER_SET_LV(LOG_LV_DEBUG);
    thread_pool::getInstance(num_tot_thread);
    // work();

    const int back_log = 20;
    const unsigned int port = 2333;
    int sd = openListenfd(port, back_log);

    int epfd = Epoll_create(1);
    Epoll_add(epfd, sd, EPOLLIN);

    LOGGER_SIMP(LOG_LV_DEBUG, "starting epoll wait");

    const int time_out_epoll = 2000;
    const int max_events_epoll = 10;
    startEpollWait(epfd, sd, time_out_epoll, max_events_epoll);

    return 0;
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
    return tmpfd;
}

// 处理客户端的HTTP请求
void handleRequest(int fd) {
    // 提交任务到线程池 TODO
    struct task_arg {
        int fd;
    };

    runnable runner = [](void *arg) {
        task_arg targ = *(task_arg *)arg;
        const int buf_size = 1024;
        char buf[buf_size];
        int n = read(targ.fd, buf, buf_size);
        char bufout[buf_size << 1] = {};
        int len = strlen(buf) + 51;
        n = snprintf(bufout, buf_size << 1, "HTTP1.0 %d OK\r\ncontent-language: en-US\r\ncontent-type: text/html; charset=utf-8\r\ncontent-length: %d\r\n\r\n%s\n<strong style=\"font-size:72px;\">\nHello World!<strong>", 200, len, buf);
        n = write(targ.fd, bufout, n);
        Close(targ.fd);
    };
    task_arg *targ = (task_arg *)malloc(sizeof(task_arg));
    targ->fd = fd;
    thread_pool::getInstance(num_tot_thread).pushTask(task(runner, (void *)targ));
}

// 开始监听文件描述符上的事件
void startEpollWait(int epfd, int sd, int time_out_epoll, int max_events_epoll) {
    LOGGER_SIMP(LOG_LV_VERBOSE, "startEpollWait");

    while (true) {
        int num_ready = 0;
        struct epoll_event evlist[max_events_epoll];
        num_ready = Epoll_wait(epfd, evlist, max_events_epoll, time_out_epoll);

        for (int i = 0; i < num_ready; i++) {
            LOGGER_FORMAT(LOG_LV_DEBUG, "fd=%d; events: %s%s%s", evlist[i].data.fd, (evlist[i].events & EPOLLIN) ? "EPOLLIN " : "", (evlist[i].events & EPOLLHUP) ? "EPOLLHUP " : "", (evlist[i].events & EPOLLERR) ? "EPOLLERR " : "");

            epoll_event tmpev = evlist[i];

            if (tmpev.events & (EPOLLRDHUP | EPOLLHUP)) { // 流套接字对端关闭连接
                Close(tmpev.data.fd);
            } else if (tmpev.events & EPOLLIN) { // 客户端连接或数据读取
                // 客户端建立连接
                if (tmpev.data.fd == sd) {
                    LOGGER_SIMP(LOG_LV_DEBUG, "epoll: new client");
                    handleRequest(acceptClient(epfd, sd));
                }
                // 客户端数据传输请求
                else {
                    LOGGER_SIMP(LOG_LV_DEBUG, "epoll: new data");
                    handleRequest(tmpev.data.fd);
                }
            } else if (tmpev.events & EPOLLERR) { //错误
                LOGGER_SIMP(LOG_LV_DEBUG, "epoll_wait error!");
                errExitSimp("epoll failed");
            }
        }
        // 睡一会，方便Debug
        LOGGER_FORMAT(LOG_LV_DEBUG, "sleep for 3 seconds, %d seconds left.", sleep(1));
    }
}
