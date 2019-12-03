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
#include <wait.h>

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
                logger::fail({__func__, " failed with string(first 20 bytes): ", s.substr(0, std::max((int)s.size(), 20))}, true);
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
                logger::fail({__func__, " failed, bytes written less than size(ret < s.size())"}, true);
                return -1;
            }
        }
    }
    // write [str_header] [fd_file] to client, using [write2client] and [sendfile]
    // won't close [sd] [fd_file]
    int writeFBundle2client(const string &str_header, const int sz_file, const int fd_file) {
        logger::verbose({"start write file bundles to client"});
        int ret = write2client(str_header);
        if (ret != -1)
            ret = sendfile(sd, fd_file, NULL, sz_file);
        if (ret == -1) {
            logger::fail({__func__, " write file fd: ", to_string(fd_file), " to client failed"}, true);
            return -1;
        }
        return 0;
    }
    // todo different file type
    // give filepath, create [header], [file_size] and **opened** [fd_file]
    // -1 for error, and fd_file is not specified
    int createFileBundle(string path_abs, string &str_header, int &sz_file, int &fd_file, int status_code = 200) {
        logger::verbose({"creating static file bundles"});
        if (access(path_abs.c_str(), F_OK | R_OK) == -1) {
            logger::fail({__func__, " access file: ", path_abs, " failed"}, true);
            return -1;
        }
        response_header header;
        response_header::htmlHeader(path_abs, header, status_code);

        fd_file = open(path_abs.c_str(), O_RDONLY);
        if (fd_file == -1) {
            logger::fail({__func__, " open ", path_abs, " failed"}, true);
            return -1;
        }
        struct stat file_info;
        int ret = fstat(fd_file, &file_info);
        if (ret == -1) {
            logger::fail({__func__, " fstat failed"}, true);
            return -1;
        }
        str_header = header.toString();
        sz_file = file_info.st_size;
        return 0;
    }
    // path_url has 'static/', -1 for internal error, 0 for success, 1 for 404
    int serveStatic(string path_abs) {
        logger::info({"start serve static file: ", path_abs});

        int ret = 0;
        // generate header
        // path is a regular file or not, -1 error, 0 not, 1 ok
        auto isRegFile = [](const char *path) -> int {
            struct stat path_info;
            int ret = stat(path, &path_info);
            if (ret == -1) {
                logger::info({" stat file failed"});
                return -1;
            }
            if (path_info.st_mode & S_IFREG) return 1;
            return 0;
        };
        ret = isRegFile(path_abs.c_str());
        if (ret == -1) { // no such file
            logger::info({path_abs, " doesnot exist"});
            return 1;
        } else if (ret == 0) { // not a regular file
            logger::info({path_abs, " not a regular file"});
            return 1;
        }
        // create bundle
        string str_header;
        int sz_file = 0;
        int fd_file = 0;
        ret = createFileBundle(path_abs, str_header, sz_file, fd_file);
        if (ret == -1) {
            logger::fail({__func__, " create file bundle failed"});
            return -1;
        }
        // do all write here
        ret = writeFBundle2client(str_header, sz_file, fd_file);
        if (ret == -1) {
            logger::fail({__func__, " write file bundle failed"});
            return -1;
        }
        if (close(sd) != -1) {
            if (close(fd_file) != -1) {
                logger::verbose({"response to socket: ", to_string(sd), " successfully written "});
                return 0;
            }
        }
        logger::fail({__func__, " close sd or fd_file failed"}, true);
        return -1;
    }
    // add headers to environ -> fork -> duplicate socket_fd to stdout -> exec
    int serveCgi(string path_abs, const request_header &header, const vector<string> list_paras) {
        // todo serve cgi
        logger::info({"start serve cgi prog: ", path_abs});
        int ret = 0;
        // prepare environ
        auto list_pair_headers = header.list_pair_headers;
        const int sz_pair_headers = list_pair_headers.size();
        char *env[sz_pair_headers + 1];
        for (int i = 0; i < sz_pair_headers; i++) {
            auto &pair_tmp = list_pair_headers[i];
            int sz_first = pair_tmp.first.size();
            int sz_second = pair_tmp.second.size();
            // create env
            env[i] = (char *)malloc(sz_first + sz_second + 1);
            strncpy(env[i], pair_tmp.first.c_str(), sz_first);
            env[i][sz_first] = ':';
            strncpy((env[i] + sz_first + 1), pair_tmp.second.c_str(), sz_second);
        }
        env[sz_pair_headers] = nullptr;
        // prepare exe file
        const char *path_cgi = config::path_file_cgi_process.c_str();
        // prepare args
        int pos_name = config::path_file_cgi_process.find_last_of('/');
        pos_name += 1;
        const char *name_tmp = config::path_file_cgi_process.substr(pos_name).c_str();
        char *name = strdup(name_tmp);
        char *argv[2] = {name, nullptr};
        ret = dup2(sd, STDOUT_FILENO);
        if (ret == -1) {
            logger::fail({__func__, " dup2 failed"}, true);
            exit(EXIT_FAILURE);
        }
        // fork is needed!!!
        ret = fork();
        if (ret > 0) { // parent
            close(sd);
            if (waitpid(ret, NULL, 0) == -1) {
                logger::fail({__func__, " waitpid failed"}, true);
                return -1;
            }
            free(name);
            return 0;
        } else if (ret == 0) { // child, dup and exec
            ret = dup2(sd, STDOUT_FILENO);
            if (ret == -1) {
                logger::fail({__func__, " dup2 failed"}, true);
                exit(EXIT_FAILURE);
            }
            execve(path_cgi, argv, env);
            logger::fail({__func__, " execve failed!"}, true);
            exit(EXIT_FAILURE);
        } else {
            logger::fail({__func__, " fork failed!"}, true);
            return -1;
        }
    }
    // todo serve different code: 403 404 500 502...
    // todo custom error file page
    int serveError(int status_code) {
        logger::info({"start serve error file, status_code: ", to_string(status_code)});
        int ret = 0;
        int fd_file;
        int sz_file;
        string str_header;
        sz_file = fd_file = 0;

        const string path_abs = config::path_url_error;
        ret = createFileBundle(path_abs, str_header, sz_file, fd_file, status_code);
        if (ret == -1) {
            logger::fail({__func__, " create file bundle failed!"});
            return -1;
        }
        ret = writeFBundle2client(str_header, sz_file, fd_file);
        if (ret == -1) {
            logger::fail({__func__, " write file bundle to client failed!"});
            return -1;
        }
        close(sd);
        close(fd_file);

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
                logger::fail({__func__, " reader.readline failed"});
                return -1;
            }
            if (ret == 0) {
                logger::fail({__func__, " reader.readline return 0"});
                return -1;
            }
            if (line == line_crlf) break;
            if (ret > 0) lines.push_back(line);
        }
        ret = request_header::fromHeaderLines(lines, header);
        if (ret == -1) {
            logger::fail({__func__, " fromHeaderLines failed with return -1"});
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
            logger::fail({__func__, " readRequestHeader failed"});
            serveError(response_header::CODE_INTERNAL_SERVER_ERROR);
            return -1;
        }
        string path;
        vector<string> list_paras;
        logger::verbose({"parse request url"});
        // pathParser(header.url, path);
        ret = utils::parse_url(header.url, path, list_paras);
        if (ret == 1) {
            // static
            ret = serveStatic(path);
            if (ret == -1) {
                logger::fail({__func__, " serve static file failed"});
                serveError(response_header::CODE_INTERNAL_SERVER_ERROR);
            } else if (ret == 1) {
                serveError(response_header::CODE_NOT_FOUND);
            }
        } else if (ret == 0) {
            // cgi
            ret = serveCgi(path, header, list_paras);
            if (ret == -1) {
                logger::fail({__func__, " serve cgi failed"});
                serveError(response_header::CODE_INTERNAL_SERVER_ERROR);
            }
        } else {
            serveError(response_header::CODE_NOT_FOUND);
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
