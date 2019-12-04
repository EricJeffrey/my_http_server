#if !defined(MAIN_LISTENER_H)
#define MAIN_LISTENER_H

#include "config.h"
#include "conn_hanlder.h"
#include "utils.h"

class main_listener {
private:
    // -1 when error
    int openListenFd(int port, int backlog) {
        addrinfo hints, *listp, *p;
        int listenfd, optval = 1;
        int ret = 0;

        memset(&hints, 0, sizeof(hints));
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_flags = AI_PASSIVE | AI_ADDRCONFIG | AI_NUMERICSERV;
        const char *port_str = to_string(port).c_str();
        getaddrinfo(NULL, port_str, &hints, &listp);
        for (p = listp; p; p = p->ai_next) {
            listenfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
            if (listenfd < 0) continue;
            // reuse
            setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval, sizeof(int));
            ret = bind(listenfd, p->ai_addr, p->ai_addrlen);
            if (ret == 0) break;
            close(listenfd);
        }
        freeaddrinfo(listp);
        if (p == nullptr) {
            logger::fail({__func__, "create listen socket failed"}, true);
            return -1;
        }
        if (listen(listenfd, backlog) < 0) {
            close(listenfd);
            logger::fail({__func__, " call to listen on socket failed"}, true);
            return -1;
        }
        return listenfd;
    }

public:
    main_listener() {}
    ~main_listener() {}

    void start() {
        int sd_listen = openListenFd(config::port, config::backlog);
        if (sd_listen == -1) {
            logger::fail({__func__, " call to open listen fd failed"}, true);
            exit(EXIT_FAILURE);
        }
        logger::info({"server started, listening on port 8686"});

        vector<thread> list_threads;
        while (true) {
            sockaddr_in addr_tmp;
            socklen_t len_addr_tmp = sizeof(addr_tmp);
            int sd_conn = accept(sd_listen, (sockaddr *)&addr_tmp, &len_addr_tmp);
            if (sd_conn == -1)
                logger::fail({__func__, " call to accept failed"}, true);
            logger::info({"connection from: ", inet_ntoa(addr_tmp.sin_addr), " established with socket: ", to_string(sd_conn)});
            list_threads.push_back(thread(&conn_handler::start, conn_handler(sd_conn, addr_tmp, len_addr_tmp)));
        }
    }
};

#endif // MAIN_LISTENER_H
