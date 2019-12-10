#if !defined(MAIN_APP_CPP)
#define MAIN_APP_CPP

#include "main_app.h"
#include "logger.h"
#include "main_listener.h"
#include <exception>
#include <regex>
#include <signal.h>

using std::regex;
using std::regex_match;

// -1 for error, -2 for help
int main_app::parseArgs() {
    auto check_port = [](string port) -> bool {
        return regex_match(port, regex("[1-9]{1}[0-9]{3,4}"));
    };
    auto check_addr = [](string addr) -> bool {
        return regex_match(addr, regex("(25[0-5]|2[0-4]\\d|[0-1]\\d{2}|[1-9]?\\d).(25[0-5]|2[0-4]\\d|[0-1]\\d{2}|[1-9]?\\d).(25[0-5]|2[0-4]\\d|[0-1]\\d{2}|[1-9]?\\d).(25[0-5]|2[0-4]\\d|[0-1]\\d{2}|[1-9]?\\d)"));
    };
    auto check_log_level = [](string log_level) -> bool {
        for (auto &&p : logger::LIST_LOG_LV2STRING)
            if (p.second == log_level) return true;
        return false;
    };
    auto check_output_path = [](string path) -> bool {
        if (path == config::path_default_logger_cerr || access(path.c_str(), F_OK | W_OK) != -1) return true;
        if (creat(path.c_str(), S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP) == -1)
            if (open(path.c_str(), O_RDWR | O_TRUNC) == -1) { return false; };
        return true;
    };
    bool debug_tmp = config::debug;
    string address_tmp = config::address;
    string port_tmp = to_string(config::port);
    string log_level_tmp;
    for (auto &&p : logger::LIST_LOG_LV2STRING)
        if (p.first == config::log_level)
            log_level_tmp = p.second;
    string output_path_tmp = config::path_logger;

    const char *options = "hda:p:l:o:";
    int opt;
    while ((opt = getopt(argc, argv, options)) != -1) {
        switch (opt) {
        case 'd':
            debug_tmp = true;
            break;
        case 'a':
            address_tmp = string(optarg);
            break;
        case 'p':
            port_tmp = string(optarg);
            break;
        case 'l':
            log_level_tmp = string(optarg);
            break;
        case 'o':
            output_path_tmp = string(optarg);
            break;
        case 'h':
            cerr << usage << endl;
            return -2;
        default:
            cerr << usage << endl;
            return -1;
        }
    }
    bool ok = check_port(port_tmp) && check_addr(address_tmp) &&
              check_log_level(log_level_tmp) && check_output_path(output_path_tmp);
    if (!ok) {
        cerr << "invalid argument" << endl;
        return -1;
    }
    config::port = std::stoi(port_tmp);
    config::address = address_tmp;
    config::debug = debug_tmp;
    for (auto &&p : logger::LIST_LOG_LV2STRING)
        if (p.second == log_level_tmp) config::log_level = p.first;
    config::path_logger = output_path_tmp;
    return 0;
}

void main_app::init() {
    app_state = state::initialize;
    { // handle sigint
        struct sigaction sa;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = 0;
        sa.sa_handler = [](int) -> void { stop(); };
        if (sigaction(SIGINT, &sa, nullptr) == -1) {
            logger::fail({"in ", __func__, " call to sigaction failed"}, true);
            return;
        }
    }
    { // config
        config::address = "127.0.0.1";
        config::port = 8686;
        config::backlog = 1024;

        config::debug = true;
        config::log_level = logger::LOG_LV_INFO;

        typedef pair<string, string> PAIR_SS;
        config::list_url2path_static = {
            PAIR_SS("/static/", "./static/"),
            PAIR_SS("/", "./"),
            PAIR_SS("/hello/", "./hello/"),
            PAIR_SS("/file/", "./file/"),
        };
        config::list_url2file_cgi = {
            PAIR_SS("/take", "./take.py"),
            PAIR_SS("/cgi/tell", "./tell.py"),
            PAIR_SS("/find", "./find.py"),
        };

        typedef pair<int, string> PAIR_IS;
        config::map_code2file_error = {
            PAIR_IS(response_header::CODE_NOT_FOUND, "./404.html"),
            PAIR_IS(response_header::CODE_INTERNAL_SERVER_ERROR, "./500.html"),
        };

        config::timeout_sec_conn = 10;

        config::key_env_query_string = "query";
        config::path_logger = "cerr";
    }
    { // parse arguments
        int ret = parseArgs();
        if (ret == -1) {
            throw std::runtime_error("invalid arguments");
        } else if (ret == -2) {
            exit(EXIT_SUCCESS);
        }
    }
    { // response phrase
        response_header::code2phrase[response_header::CODE_OK] = "OK";
        response_header::code2phrase[response_header::CODE_NOT_FOUND] = "Not Found";
        response_header::code2phrase[response_header::CODE_INTERNAL_SERVER_ERROR] = "Internal Server Error";

        response_header::STR_VERSION_HTTP_1_1 = "HTTP/1.1";
    }
}
void main_app::start() {
    app_state = state::start;
    logger::start();
    main_listener().start();
}
void main_app::stop() {
    app_state = state::stop;
    logger::info({"server stopping, bye"});
    logger::stop();
    for (auto &&pid : set_child_pids)
        kill(pid, SIGINT);
    exit(0);
}

state main_app::app_state;
set<pid_t> main_app::set_child_pids;

int main_app::argc;
char **main_app::argv;
const string main_app::usage = "\nhalox - yet another http server\n\
Usage: halox [-h] [-d] [-a addr] [-p port] [-l loglevel] [-o logfile]\n\n\
Options:\n\
    -d\t\t: print debug message\n\
    -a addr\t: set address to listen on, default 127.0.0.1\n\
    -p port\t: set port to listen on, default 8686\n\
    -l loglevel\t: set log level, 'info'(default) or 'verbose'\n\
    -o logfile\t: set log file path, default stderr\n";

#endif // MAIN_APP_CPP
