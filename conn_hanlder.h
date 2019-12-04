#if !defined(CONN_HANDLER_H)
#define CONN_HANDLER_H

#include "buffered_reader.h"
#include "cgi_server.h"
#include "config.h"
#include "request_header.h"
#include "response_header.h"
#include "static_server.h"
#include "utils.h"
#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>

using std::thread;

class conn_handler {
private:
    int sd;
    sockaddr_in addr;
    socklen_t len_addr;

    // todo serve different code: 404 500...
    // todo custom error file page
    int serveError(int status_code) {
        logger::info({"start serve error file, status_code: ", to_string(status_code)});

        int ret = 0;
        const auto &mp = config::map_code2file_error;
        if (mp.find(status_code) == mp.end()) {
            // unsupported, just send code
            response_header header;
            const string data = "error";
            response_header::strHeader(data, header, status_code);
            string resp = header.toString() + data;
            ret = utils::writeStr2Fd(resp, sd);
            if (ret == -1) {
                logger::fail({__func__, " call to utils.writeStr2Fd failed"});
                return -1;
            }
        }

        int fd_file;
        int sz_file;
        string str_header;
        sz_file = fd_file = 0;
        const string path_abs = mp.at(status_code);
        ret = createFileBundle(path_abs, str_header, sz_file, fd_file, status_code);
        if (ret == -1) {
            logger::fail({__func__, " call to create file bundle failed!"});
            return -1;
        }
        ret = writeFBundle2client(str_header, sz_file, fd_file, sd);
        if (ret == -1) {
            logger::fail({__func__, " call to write file bundle to client failed!"});
            return -1;
        }
        close(fd_file);

        return 0;
    }
    // clear lines. do not read body
    // return num of lines readed, -1 for error, -2 for timeout
    int readRequestHeader(request_header &header) {
        logger::info({"reading request from socket: ", to_string(sd)});

        buffered_reader reader(sd);
        int ret = 0;
        // check header end
        const string line_crlf = string({'\r', '\n'});
        vector<string> lines;
        while (true) {
            string line;
            ret = reader.readLine(line);
            if (ret == -1) {
                logger::fail({__func__, " call to reader.readline on sd: ", to_string(sd), " failed"});
                return -1;
            } else if (ret == -2) {
                logger::verbose({"reader.readline Timeout"});
                return -2;
            } else if (ret == 0) {
                logger::fail({__func__, " call to reader.readline on sd: ", to_string(sd), " return 0"});
                return -1;
            }
            if (line == line_crlf) break;
            if (ret > 0) lines.push_back(line);
        }
        ret = request_header::fromHeaderLines(lines, header);
        if (ret == -1) {
            logger::fail({__func__, " call to fromHeaderLines failed with return -1"});
            return -1;
        };
        return 0;
    }
    // run in new thread!
    int handleNewConn() {
        logger::info({"handling new conn from: ", inet_ntoa(addr.sin_addr)});
        int ret = 0;

        // set socket read timeout
        struct timeval tv;
        tv.tv_sec = (decltype(tv.tv_sec))(config::timeout_sec_sock);
        tv.tv_usec = (decltype(tv.tv_usec))((config::timeout_sec_sock - tv.tv_sec) * 1000);
        ret = setsockopt(sd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof tv);
        if (ret == -1) {
            logger::fail({__func__, " call to setsockopt failed"}, true);
            return -1;
        }

        clock_t clock_start = clock();
        while ((clock() - clock_start) / (double)CLOCKS_PER_SEC < config::timeout_sec_conn) {
            request_header header;
            ret = readRequestHeader(header);
            if (ret == -1) {
                logger::fail({__func__, " call to readRequestHeader failed"});
                serveError(response_header::CODE_INTERNAL_SERVER_ERROR);
                return -1;
            } else if (ret == -2) {
                // timeout
                logger::info({__func__, " call to readRequestHeader failed, socket: ", to_string(sd), " Timeout"});
                close(sd);
                return -1;
            }
            logger::debug({"request url: ", header.url});
            string path;
            vector<string> list_paras;
            logger::verbose({"parse request url"});
            ret = utils::parseUrl(header.url, path, list_paras);
            if (ret == 1) {
                // static
                ret = serveStatic(path, sd);
                if (ret == -1) {
                    logger::fail({__func__, " call to serve static file failed"});
                    serveError(response_header::CODE_INTERNAL_SERVER_ERROR);
                } else if (ret == 1) {
                    serveError(response_header::CODE_NOT_FOUND);
                }
            } else if (ret == 0) {
                // cgi
                ret = serveCgi(path, list_paras, sd);
                if (ret == -1) {
                    logger::fail({__func__, " call to serve cgi failed"});
                    serveError(response_header::CODE_INTERNAL_SERVER_ERROR);
                } else if (ret == 1) {
                    logger::info({"no cgi prog found"});
                    serveError(response_header::CODE_NOT_FOUND);
                }
            } else {
                serveError(response_header::CODE_NOT_FOUND);
            }
        }
        ret = close(sd);
        if (ret < 0) {
            logger::fail({__func__, " call to close failed"}, true);
            return -1;
        }
        return 0;
    }

public:
    conn_handler(int sd, sockaddr_in addr, socklen_t len_addr) {
        this->sd = sd;
        this->addr = addr;
        this->len_addr = len_addr;
    }
    ~conn_handler() {}

    void start() { handleNewConn(); }
};

#endif // CONN_HANDLER_H
