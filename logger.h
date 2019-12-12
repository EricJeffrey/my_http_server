#if !defined(LOGGER_H)
#define LOGGER_H

#include "config.h"
#include <cerrno>
#include <cstring>
#include <fstream>
#include <initializer_list>
#include <iostream>
#include <mutex>
#include <ostream>
#include <queue>
#include <sstream>
#include <string>
#include <sys/syscall.h>
#include <sys/types.h>
#include <thread>
#include <unistd.h>

using std::cerr;
using std::endl;
using std::initializer_list;
using std::mutex;
using std::ofstream;
using std::ostream;
using std::queue;
using std::string;
using std::stringstream;
using std::thread;
using std::unique_ptr;

class logger {
private:
    static thread thread_logger;
    static mutex mtx_logger;
    static queue<string> queue_logger;
    static bool running_logger;
    static ostream *ofs_logger;
    static bool ofs_using_cerr;

    // get current thread id
    static void preInfo(stringstream &ss) {
        ss << syscall(__NR_gettid) << " ";
    }
    static void msgInfo(stringstream &ss, initializer_list<string> &msg_list) {
        for (auto &&msg : msg_list) ss << msg;
    }
    // lock and flush queue
    static void flushQueue() {
        mtx_logger.lock();
        while (!queue_logger.empty()) {
            (*ofs_logger) << queue_logger.front() << endl;
            queue_logger.pop();
        }
        mtx_logger.unlock();
        ofs_logger->flush();
    }
    // todo maybe too many lock/unlock, consider another way
    // commit log
    static void commitLog(const string &s) {
        mtx_logger.lock();
        queue_logger.push(s);
        mtx_logger.unlock();
    }

public:
    static const int LOG_LV_VERBOSE;
    static const int LOG_LV_INFO;
    static const vector<PAIR_IS> LIST_LOG_LV2STRING;

    logger() {}
    ~logger() {}

    // to [stderr]
    static void info(initializer_list<string> msg_list) {
        if (config::log_level >= LOG_LV_INFO) {
            stringstream ss;
            preInfo(ss);
            ss << "info\t";
            msgInfo(ss, msg_list);
            commitLog(ss.str());
        }
    }
    // to [stderr]
    static void verbose(initializer_list<string> msg_list) {
        if (config::log_level > LOG_LV_VERBOSE) {
            stringstream ss;
            preInfo(ss);
            ss << "verbose\t";
            msgInfo(ss, msg_list);
            commitLog(ss.str());
        }
    }
    // to [stderr], print [errno] if [show_errno]=true, no [exit]!
    static void fail(initializer_list<string> msg_list, bool show_erron = false) {
        int errno_tmp = errno;
        string err_msg = string(strerror(errno_tmp));

        stringstream ss;
        preInfo(ss);
        ss << "ERROR\t";
        msgInfo(ss, msg_list);
        if (show_erron && errno_tmp != 0)
            ss << ". errno: " << errno_tmp << ", " << err_msg << ".";
        commitLog(ss.str());
    }
    // to [stderr]
    static void debug(initializer_list<string> msg_list) {
        if (config::debug) {
            stringstream ss;
            preInfo(ss);
            ss << "debug\t";
            msgInfo(ss, msg_list);
            commitLog(ss.str());
        }
    }

    // stop thread, flush queue
    static void stop() {
        running_logger = false;
        flushQueue();
    }
    // start logger thread
    static void start() {
        if (config::path_logger == config::path_default_logger_cerr) {
            ofs_logger = &cerr, ofs_using_cerr = true;
        } else {
            ofs_logger = new ofstream(config::path_logger, std::ios_base::trunc);
        }
        running_logger = true;
        thread_logger = thread([]() -> void {
            // sleep for 0.2 seconds
            struct timespec ts = {200000000, 0}, remain_ts;
            while (running_logger) {
                ts.tv_nsec = 200000000, ts.tv_sec = 0;
                nanosleep(&ts, &remain_ts);
                flushQueue();
            }
            if (!ofs_using_cerr)
                dynamic_cast<ofstream *>(ofs_logger)->close();
        });
        thread_logger.detach();
    }
};
const int logger::LOG_LV_VERBOSE = 5;
const int logger::LOG_LV_INFO = 4;
const vector<PAIR_IS> logger::LIST_LOG_LV2STRING = {
    PAIR_IS(LOG_LV_INFO, "info"),
    PAIR_IS(LOG_LV_VERBOSE, "verbose"),
};

thread logger::thread_logger;
mutex logger::mtx_logger;
queue<string> logger::queue_logger;
bool logger::running_logger = false;
ostream *logger::ofs_logger;
bool logger::ofs_using_cerr = false;

#endif // LOGGER_H
