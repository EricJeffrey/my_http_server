#if !defined(CONN_HANDLER_H)
#define CONN_HANDLER_H

#include "buffered_reader.h"
#include "config.h"
#include "request_header.h"
#include "response_header.h"
#include "utils.h"
#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <sys/sendfile.h>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>

using std::thread;

class conn_handler {
private:
    int sd;
    sockaddr_in addr;
    socklen_t len_addr;

    int write2client(const string &s) {
        const char *buf = s.c_str();
        int sz = s.size();
        while (true) {
            int ret = write(sd, buf, sz);
            // all bytes written
            if (ret == sz) return 0;
            if (ret == -1) {
                logger::fail({__func__, "failed with string(first 20 bytes): ", s.substr(0, std::max((int)s.size(), 20))}, true);
                return -1;
            }
            if (ret < sz) {
                // interrupted by signal
                int errno_tmp = errno;
                if (errno_tmp == EINTR) {
                    buf += ret;
                    sz -= ret;
                    continue;
                }
                logger::fail({__func__, "failed, bytes written less than size(ret < s.size())"}, true);
                return -1;
            }
        }
    }
    // write [str_header] [fd_file] to client, using [write2client] and [sendfile]
    // do not close [sd] [fd_file]
    int writeFBundle2client(const string &str_header, const int sz_file, const int fd_file) {
        logger::verbose({"start write file bundles to client"});
        int ret = write2client(str_header);
        if (ret != -1)
            ret = sendfile(sd, fd_file, NULL, sz_file);
        if (ret == -1) {
            logger::fail({__func__, " write file fd: ", to_string(fd_file), " to client failed"}, true);
            serveError(response_header::CODE_INTERNAL_SERVER_ERROR);
            return -1;
        }
        return 0;
    }
    // give filepath, create [header], [file_size] and **opened** [fd_file]
    // -1 for error, and fd_file is not specified
    int createFileBundle(string path_abs, string &str_header, int &sz_file, int &fd_file, int status_code = 200) {
        logger::verbose({"creating static file bundles"});
        if (access(path_abs.c_str(), F_OK | R_OK) == -1) {
            logger::fail({__func__, "access file: ", path_abs, " failed"}, true);
            return -1;
        }
        response_header header;
        response_header::htmlHeader(path_abs, header, status_code);

        fd_file = open(path_abs.c_str(), O_RDONLY);
        if (fd_file == -1) {
            logger::fail({__func__, "open ", path_abs, " failed"}, true);
            return -1;
        }
        struct stat file_info;
        int ret = fstat(fd_file, &file_info);
        if (ret == -1) {
            logger::fail({__func__, "fstat failed"}, true);
            return -1;
        }
        str_header = header.toString();
        sz_file = file_info.st_size;
        return 0;
    }
    // path_url has 'static/', always close sd (unless cannot), send 500 when error
    int serveStatic(string path_url) {
        logger::info({"start serve file with url: ", path_url});

        int ret = 0;
        // generate header
        int sz_tmp = config::path_url_static.size();
        string path_abs = config::path_dir_static_root + path_url.substr(sz_tmp, path_url.size() - sz_tmp);
        // no file or cannot read
        if (access(path_abs.c_str(), F_OK | R_OK) == -1) {
            logger::fail({__func__, "access file: ", path_abs, " failed"}, true);
            serveError(response_header::CODE_NOT_FOUND);
            return -1;
        }
        // create bundle
        string str_header;
        int sz_file = 0;
        int fd_file = 0;
        ret = createFileBundle(path_abs, str_header, sz_file, fd_file);
        if (ret == -1) {
            logger::fail({__func__, "create file bundle failed"});
            serveError(response_header::CODE_INTERNAL_SERVER_ERROR);
            return -1;
        }
        // do all write here
        ret = writeFBundle2client(str_header, sz_file, fd_file);
        if (ret == -1) {
            logger::fail({__func__, "write file bundle failed"});
            return -1;
        }
        close(sd);
        close(fd_file);
        logger::verbose({"response to socket: ", to_string(sd), " successfully written "});
        return 0;
    }
    // todo serve cgi
    int serveCgi(string path_url) {
        //
    }
    // todo serve 404 500...
    int serveError(int status_code) {
        logger::info({"start serve error file, status_code: ", to_string(status_code)});
        int ret = 0;
        int fd_file;
        int sz_file;
        string str_header;
        sz_file = fd_file = 0;

        const string path_abs = string("./static/error.html", 19);
        ret = createFileBundle(path_abs, str_header, sz_file, fd_file, status_code);
        if (ret == -1) {
            logger::fail({__func__, "create file bundle failed!"});
            return -1;
        }
        ret = writeFBundle2client(str_header, sz_file, fd_file);
        if (ret == -1) {
            logger::fail({__func__, "write file bundle to client failed!"});
            return -1;
        }
        close(sd);
        close(fd_file);

        return 0;
    }
    // get path, without first '/'
    int pathParser(string url, string &path) {
        if (url == string({'/'}))
            url = string("/static/index.html", 18);
        else if (url == string("/favicon.ico", 12))
            url = string("/static/favicon.icon", 20);
        // just delete first '/'
        path = url.substr(1, url.size() - 1);
        return 0;
    }
    // return num of lines readed, -1 for error, clear lines. do not read body
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
            logger::debug({"read request header readline got: ", line});
            if (ret < 0) {
                logger::fail({__func__, "reader.readline failed"});
                return -1;
            }
            if (ret == 0) {
                logger::fail({__func__, "reader.readline return 0"});
                return -1;
            }
            if (ret > 0) lines.push_back(line);
            if (line == line_crlf) break;
        }
        ret = request_header::fromHeaderLines(lines, header);
        if (ret == -1) {
            logger::fail({__func__, "fromHeaderLines failed with return -1"});
            return -1;
        };
        return 0;
    }
    // run in new thread!
    int handleNewConn() {
        logger::info({"handling new conn from: ", inet_ntoa(addr.sin_addr)});

        request_header header;
        int ret = 0;
        ret = readRequestHeader(header);
        if (ret == -1) {
            logger::fail({__func__, "readRequestHeader failed"});
            serveError(response_header::CODE_INTERNAL_SERVER_ERROR);
            return -1;
        }
        string path;
        logger::verbose({"parse request url"});
        logger::debug({"request url: ", header.url});
        pathParser(header.url, path);
        const int sz_path_tmp = path.size();
        if (sz_path_tmp > 0 && path.find(config::path_url_static) == 0) {
            serveStatic(path);
        } else if (sz_path_tmp > 0 && path.find(config::path_url_cgi) == 0) {
            serveCgi(path);
        } else {
            serveError(response_header::CODE_NOT_FOUND);
        }

        { // temp response
            // auto sendResp = [this]() -> void {
            //     std::stringstream ss;
            //     ss << "HTTP/1.1 200 OK\r\n"
            //        << "content-length:" << 5 << "\r\n\r\n"
            //        << "hello";
            //     string rep = ss.str();
            //     write(sd, rep.c_str(), rep.length());
            // };
            // sendResp();
            // close(sd);
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

    void start() {
        handleNewConn();
    }
};

#endif // CONN_HANDLER_H
