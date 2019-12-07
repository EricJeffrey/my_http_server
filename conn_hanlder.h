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
            const string data = "error" + to_string(status_code);
            response_header::strHeader(data, header, status_code);
            string resp = header.toString() + data;
            ret = utils::writeStr2Fd(resp, sd);
            if (ret == -1) {
                logger::fail({"in ", __func__, ": call to utils.writeStr2Fd failed"});
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
            logger::fail({"in ", __func__, ": call to create file bundle failed!"});
            return -1;
        }
        ret = writeFBundle2client(str_header, sz_file, fd_file, sd);
        if (ret == -1) {
            logger::fail({"in ", __func__, ": call to write file bundle to client failed!"});
            return -1;
        }
        close(fd_file);

        return 0;
    }
    // clear lines. do not read body
    // return num of lines readed, -1 for error, -2 for wouldblock, -3 for closed
    int readRequestHeader(request_header &header) {

        buffered_reader reader(sd);
        int ret = 0;
        // check header end
        const string line_crlf = string("\r\n");
        vector<string> lines;
        while (true) {
            string line;
            ret = reader.readLine(line);
            if (ret == -1) {
                logger::fail({"in ", __func__, ": call to reader.readline on sd: ", to_string(sd), " failed"});
                return -1;
            } else if (ret == -2) {
                return -2;
            } else if (ret == 0) {
                logger::info({"in ", __func__, ": call to reader.readline on sd: ", to_string(sd), " return 0, closed"});
                return -3;
            }
            if (line == line_crlf) break;
            if (ret > 0) lines.push_back(line);
        }
        ret = request_header::fromHeaderLines(lines, header);
        if (ret == -1) {
            logger::fail({"in ", __func__, ": call to fromHeaderLines failed with return -1"});
            return -1;
        };
        return 0;
    }
    // run in new thread!
    int handleNewConn() {
        logger::info({"handling new conn from sock: ", to_string(sd), " , addr: ", inet_ntoa(addr.sin_addr)});
        int ret = 0;

        // set to nonblock
        ret = fcntl(sd, F_SETFL, O_NONBLOCK);
        if (ret == -1) {
            logger::fail({"in ", __func__, ": call to fcntl, set socket nonblock failed"}, true);
            close(sd);
            return -1;
        }
        // timer
        clock_t clock_start = clock();
        while ((clock() - clock_start) / (double)CLOCKS_PER_SEC < config::timeout_sec_conn) {
            request_header header;
            ret = readRequestHeader(header);
            if (ret == -1) {
                logger::fail({"in ", __func__, ": call to readRequestHeader failed"});
                serveError(response_header::CODE_INTERNAL_SERVER_ERROR);
                return -1;
            } else if (ret == -2) {
                // would block, continue
                continue;
            } else if (ret == -3) {
                logger::info({"connection to socket: ", to_string(sd), " closed"});
                close(sd);
                return 0;
            }
            logger::debug({"request url: ", header.url});
            string path;
            vector<string> list_paras;
            ret = utils::parseUrl(header.url, path, list_paras);
            if (ret == 1) {
                // static
                ret = serveStatic(path, sd);
                if (ret == -1) {
                    logger::fail({"in ", __func__, ": call to serveStatic failed"});
                    serveError(response_header::CODE_INTERNAL_SERVER_ERROR);
                } else if (ret == -2) {
                    serveError(response_header::CODE_NOT_FOUND);
                }
            } else if (ret == 0) {
                // cgi
                ret = serveCgi(path, list_paras, sd);
                if (ret == -1) {
                    logger::fail({"in ", __func__, ": call to serveCgi failed"});
                    serveError(response_header::CODE_INTERNAL_SERVER_ERROR);
                } else if (ret == -2) {
                    serveError(response_header::CODE_NOT_FOUND);
                }
            } else {
                serveError(response_header::CODE_NOT_FOUND);
            }
        }
        logger::verbose({"connection from socket: ", to_string(sd), " timeout, closing"});
        ret = close(sd);
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
