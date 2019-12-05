#if !defined(STATIC_SERVER_H)
#define STATIC_SERVER_H

#include "config.h"
#include "logger.h"
#include "response_header.h"
#include "utils.h"
#include <sys/sendfile.h>

// write [str_header] [fd_file] to client, using [write2client] and [sendfile]
// won't close [sd] [fd_file]
int writeFBundle2client(const string &str_header, const int sz_file, const int fd_file, const int sd) {
    logger::verbose({"start write file bundles to client"});
    int ret = utils::writeStr2Fd(str_header, sd);
    if (ret != -1)
        ret = sendfile(sd, fd_file, NULL, sz_file);
    if (ret == -1) {
        logger::fail({__func__, " call to  call to  write file fd: ", to_string(fd_file), " to client failed"}, true);
        return -1;
    }
    return 0;
}
// todo different file type
// give filepath, create [header], [file_size] and **OPENED** [fd_file]
// -1 for error, and fd_file is not specified
int createFileBundle(const string &path_abs, string &str_header, int &sz_file, int &fd_file, int status_code = 200) {
    logger::verbose({"creating static file bundles"});
    if (access(path_abs.c_str(), F_OK | R_OK) == -1) {
        logger::fail({__func__, " call to  access file: ", path_abs, " failed"}, true);
        return -1;
    }
    response_header header;
    response_header::htmlHeader(path_abs, header, status_code);

    fd_file = open(path_abs.c_str(), O_RDONLY);
    if (fd_file == -1) {
        logger::fail({__func__, " call to  open ", path_abs, " failed"}, true);
        return -1;
    }
    struct stat file_info;
    int ret = fstat(fd_file, &file_info);
    if (ret == -1) {
        logger::fail({__func__, " call to  fstat failed"}, true);
        return -1;
    }
    str_header = header.toString();
    sz_file = file_info.st_size;
    return 0;
}
// path_url has 'static/', -1 for internal error, 0 for success, 1 for 404
int serveStatic(string path_abs, const int sd) {
    logger::info({"start serve static file: ", path_abs});

    int ret = 0;
    // generate header
    ret = utils::isRegFile(path_abs.c_str());
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
        logger::fail({__func__, " call to  create file bundle failed"});
        return -1;
    }
    // do all write here
    ret = writeFBundle2client(str_header, sz_file, fd_file, sd);
    if (ret == -1) {
        logger::fail({__func__, " call to  write file bundle failed"});
        return -1;
    }
    if (close(fd_file) != -1) {
        logger::verbose({"response to socket: ", to_string(sd), " successfully written "});
        return 0;
    }
    logger::fail({__func__, " call to  close sd or fd_file failed"}, true);
    return -1;
}

#endif // STATIC_SERVER_H
