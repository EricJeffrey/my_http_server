#if !defined(LOGGER_H)
#define LOGGER_H

#include "config.h"
#include <cerrno>
#include <cstring>
#include <initializer_list>
#include <iostream>
#include <sstream>
#include <string>
#include <sys/syscall.h>
#include <sys/types.h>
#include <unistd.h>

using std::cerr;
using std::cin;
using std::cout;
using std::endl;
using std::initializer_list;
using std::string;
using std::stringstream;

class logger {
private:
    static void preInfo(stringstream &ss) {
        pid_t tid = syscall(__NR_gettid);
        ss << tid << " ";
    }
    static void msgInfo(stringstream &ss, initializer_list<string> &msg_list) {
        for (auto &&msg : msg_list)
            ss << msg;
    }

public:
    static const int LOG_LV_VERBOSE;
    static const int LOG_LV_INFO;

    logger() {}
    ~logger() {}

    // to [stderr]
    static void info(initializer_list<string> msg_list) {
        if (config::log_level >= LOG_LV_INFO) {
            stringstream ss;
            preInfo(ss);
            ss << "log\t";
            msgInfo(ss, msg_list);
            cerr << ss.str() << endl;
        }
    }
    // to [stderr]
    static void verbose(initializer_list<string> msg_list) {
        if (config::log_level > LOG_LV_VERBOSE) {
            stringstream ss;
            preInfo(ss);
            ss << "verbose\t";
            msgInfo(ss, msg_list);
            cerr << ss.str() << endl;
        }
    }
    // to [stderr], print [errno] if [show_errno]=true, no [exit]!
    static void fail(initializer_list<string> msg_list, bool show_erron = false) {
        int errno_tmp = errno;
        string err_msg = string(strerror(errno_tmp));

        stringstream ss;
        preInfo(ss);
        ss << "ERROR\t";
        if (show_erron && errno_tmp != 0)
            ss << "errno: " << errno_tmp << ", " << err_msg << '.';
        msgInfo(ss, msg_list);
        cerr << ss.str() << '.' << endl;
    }
    // to [stderr]
    static void debug(initializer_list<string> msg_list) {
        if (config::debug) {
            stringstream ss;
            preInfo(ss);
            ss << "debug\t";
            msgInfo(ss, msg_list);
            cerr << ss.str() << endl;
        }
    }
};
const int logger::LOG_LV_VERBOSE = 5;
const int logger::LOG_LV_INFO = 4;

#endif // LOGGER_H
