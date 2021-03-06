#if !defined(STATIC_SERVER_H)
#define STATIC_SERVER_H

#include "config.h"
#include "logger.h"
#include "response_header.h"
#include "utils.h"
#include <sys/sendfile.h>
#include <sys/socket.h>
#include <sys/types.h>

// write [n] bytes of [buffer] to [sd], return n or -1
int writeBufferToSocket(int sd, void *buffer, size_t n, int flags = 0) {
    int ret = 0;
    ssize_t size_sent = 0;
    while (size_sent < (ssize_t)n) {
        ret = send(sd, (char *)buffer + (ssize_t)size_sent, n - size_sent, flags);
        if (ret == -1) {
            logger::fail({"in ", __func__, ": call to send failed"}, true);
            return -1;
        }
        size_sent += ret;
    }
    return n;
}

// read from [in_fd] and [send] them on out_sd, return 0 or -1
int writeFileToSocket(int in_fd, int out_sd, const int sz_file) {
    int ret = 0;
    const int buf_size = 8192;
    char buffer[buf_size];
    ssize_t size_sent = 0;
    while (size_sent < sz_file) {
        ret = read(in_fd, buffer, buf_size);
        if (ret < 0) {
            logger::fail({"in ", __func__, ": call to read failed"}, true);
            return -1;
        }
        int flags = (size_sent + ret >= sz_file ? 0 : MSG_MORE);
        ret = writeBufferToSocket(out_sd, buffer, ret, flags);
        if (ret < 0) {
            logger::fail({"in ", __func__, ": call to writeBufferToSocket failed"}, true);
            return -1;
        }
        size_sent += ret;
    }
    return 0;
}

// write [str_header] [fd_file] to client
// won't close [sd] [fd_file]
int writeFBundle2client(const string &str_header, const int sz_file, const int fd_file, const int sd) {
    int ret = utils::writeStr2Fd(str_header, sd);
    if (ret >= 0) ret = writeFileToSocket(fd_file, sd, sz_file);
    if (ret == -1) {
        logger::fail({"in ", __func__, ": call to  write file fd: ", to_string(fd_file), " to client failed"}, true);
        return -1;
    }
    return 0;
}
// todo different file type
// give filepath, create [header], [file_size] and **OPENED** [fd_file]
// -1 for error, and fd_file is not specified
int createFileBundle(const string &path_abs, string &str_header, int &sz_file, int &fd_file, int status_code = 200) {
    if (access(path_abs.c_str(), F_OK | R_OK) == -1) {
        logger::fail({"in ", __func__, ": call to  access file: ", path_abs, " failed"}, true);
        return -1;
    }
    response_header header;
    response_header::fileHeader(path_abs, header, status_code);

    fd_file = open(path_abs.c_str(), O_RDONLY);
    if (fd_file == -1) {
        logger::fail({"in ", __func__, ": call to  open ", path_abs, " failed"}, true);
        return -1;
    }
    struct stat file_info;
    int ret = fstat(fd_file, &file_info);
    if (ret == -1) {
        logger::fail({"in ", __func__, ": call to  fstat failed"}, true);
        return -1;
    }
    str_header = header.toString();
    sz_file = file_info.st_size;
    return 0;
}
// path_url has 'static/', -1 for internal error, 0 for success, -2 for 404
int serveStatic(string path_abs, const int sd) {
    logger::info({"start serve static file: ", path_abs});

    int ret = 0;
    // generate header
    ret = utils::isRegFile(path_abs.c_str());
    if (ret == -1) { // no such file
        logger::info({path_abs, " does not exist"});
        return -2;
    } else if (ret == 0) { // not a regular file
        logger::info({path_abs, " not a regular file"});
        return -2;
    }
    // create bundle
    string str_header;
    int sz_file = 0;
    int fd_file = 0;
    ret = createFileBundle(path_abs, str_header, sz_file, fd_file);
    if (ret == -1) {
        logger::fail({"in ", __func__, ": call to  create file bundle failed"});
        return -1;
    }
    // do all write here
    ret = writeFBundle2client(str_header, sz_file, fd_file, sd);
    if (ret == -1) {
        logger::fail({"in ", __func__, ": call to  write file bundle failed"});
        return -1;
    }
    if (close(fd_file) != -1) {
        logger::verbose({"response to socket: ", to_string(sd), " successfully written "});
        return 0;
    }
    logger::fail({"in ", __func__, ": call to  close sd or fd_file failed"}, true);
    return -1;
}

#endif // STATIC_SERVER_H
