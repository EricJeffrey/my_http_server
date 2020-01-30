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

    // 解析url路径与参数，返回路径类型
    // 1-static，0-cgi，-1-error
    static int parseUrl(string url, string &path, vector<string> &paras_list) {
        path.clear();
        paras_list.clear();
        const size_t npos = string::npos;
        const size_t sz_url = url.size();
        if (sz_url == 0) return -1;
        size_t pos_of_question_sign = url.find_first_of('?');
        int res = -1;

        size_t st_path = 0;
        size_t ed_path = (pos_of_question_sign == npos ? sz_url : pos_of_question_sign);
        // static or cgi
        int type_static = -1;
        string dir_static_mapped;
        string file_cgi_mapped;
        // static dir url
        // todo fix bug: '/' is mapped to everyone
        for (auto &&url2path : config::list_url2path_static) {
            const string &url_key_tmp = url2path.first;
            const size_t sz_url_tmp = url_key_tmp.size();
            if (sz_url > sz_url_tmp && url.substr(0, sz_url_tmp) == url_key_tmp) {
                st_path = sz_url_tmp;
                dir_static_mapped = url2path.second;
                type_static = 1;
                break;
            }
        }
        if (type_static == -1) {
            // cgi file url
            for (auto &&url2file : config::list_url2file_cgi) {
                const string &url_key_tmp = url2file.first;
                const size_t sz_url_tmp = url_key_tmp.size();
                // e.g. url='/abcd', url2file='/abc'->'xxx'
                if (sz_url >= sz_url_tmp && url.substr(0, sz_url_tmp) == url_key_tmp &&
                    (sz_url == sz_url_tmp || url[sz_url_tmp] == '?')) {
                    file_cgi_mapped = url2file.second;
                    type_static = 0;
                    break;
                }
            }
        }

        res = type_static;
        if (res == -1) return -1;
        // /static/? or /static/
        if (type_static == 1 && st_path == ed_path) return -1;
        path = (type_static == 1 ? dir_static_mapped + url.substr(st_path, ed_path - st_path) : file_cgi_mapped);
        // no ? found in url
        if (pos_of_question_sign == npos || pos_of_question_sign == sz_url - 1) return res;
        size_t st_para = pos_of_question_sign + 1;
        while (true) {
            size_t pos_and_sign = url.find_first_of('&', st_para);
            // no more '&'
            if (pos_and_sign == string::npos || pos_and_sign == sz_url) {
                paras_list.push_back(url.substr(st_para, sz_url - st_para));
                break;
            } else {
                if (pos_and_sign > st_para)
                    paras_list.push_back(url.substr(st_para, pos_and_sign - st_para));
                st_para = pos_and_sign + 1;
            }
        }
        return res;
    }
    // todo custom error file page
    int serveError(int status_code) {
        logger::info({"start serve error file, status_code: ", to_string(status_code)});

        int ret = 0;
        auto &mp = config::map_code2file_error;
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
            return 0;
        }

        int fd_file;
        int sz_file;
        string str_header;
        sz_file = fd_file = 0;
        const string &path_abs = mp[status_code];
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
        // ret = fcntl(sd, F_SETFL, O_NONBLOCK);
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
            ret = parseUrl(header.url, path, list_paras);
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
