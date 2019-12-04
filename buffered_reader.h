#if !defined(BUFFERED_READER_H)
#define BUFFERED_READER_H

#include "logger.h"
#include "utils.h"
#include <unistd.h>
#include <vector>

using std::fill;
using std::to_string;

class buffered_reader {
private:
    static const int SIZE_BUFFER = 2048;
    char buffer[SIZE_BUFFER];
    int fd;
    int pos;
    int len;

    void resetBuffer() {
        fill(buffer, buffer + SIZE_BUFFER, 0);
        pos = len = 0;
    }

    // clear buffer, return [length], -1 for error
    int fillBuffer() {
        resetBuffer();
        int ret = 0;
        int pos_tmp = 0;
        int sz_to_read = SIZE_BUFFER;
        while (true) {
            ret = read(fd, buffer + pos_tmp, sz_to_read);
            int errno_tmp = errno;
            if (ret == -1) {
                logger::fail({__func__, " call to read failed"}, true);
                return -1;
            }
            if (ret < sz_to_read) {
                if (errno_tmp == EINTR) {
                    // interrupted
                    sz_to_read -= ret;
                    pos_tmp += ret;
                    continue;
                } else {
                    // no more
                    len = pos_tmp + ret;
                    return len;
                }
            } else {
                len = SIZE_BUFFER;
                return len;
            }
        }
        return 0;
    }

public:
    buffered_reader(int fd) {
        fill(buffer, buffer + SIZE_BUFFER, 0);
        this->fd = fd;
        pos = len = 0;
    }
    ~buffered_reader() {}

    // save '\r\n', return [length], -1 for error. !! [line] will be cleared!
    int readLine(string &line) {
        line.clear();
        stringstream ss;
        int ret = 0;
        if (len == 0) {
            ret = fillBuffer();
            logger::debug({__func__, "read line fillbuffer return: ", to_string(ret)});
            if (ret == -1) {
                logger::fail({__func__, " call to fillbuffer failed"});
                return -1;
            }
            if (ret == 0) {
                logger::fail({__func__, " call to fillbuffer return 0"});
                return 0;
            }
        }
        for (int ed = pos;; ed++) {
            if (ed + 1 < len && buffer[ed] == '\r' && buffer[ed + 1] == '\n') {
                ed = ed + 2;
                ss << string(buffer + pos, buffer + ed);
                pos = ed;
                string str_tmp = ss.str();
                line = str_tmp;
                return line.size();
            }
            if (ed == len - 1) {
                ss << string(buffer + pos, buffer + len);
                ret = fillBuffer();
                if (ret == -1) {
                    logger::fail({__func__, " call to fillbuffer failed in loop"});
                    return -1;
                }
                if (ret == 0) {
                    logger::fail({__func__, " call to fillbuffer return 0, no crlf meet"});
                    return 0;
                }
            }
        }
        return 0;
    }
};

#endif // BUFFERED_READER_H
