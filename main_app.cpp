#if !defined(MAIN_APP_CPP)
#define MAIN_APP_CPP

#include "main_app.h"
#include "logger.h"
#include "main_listener.h"
#include <exception>
#include <fstream>
#include <istream>
#include <regex>
#include <signal.h>

using std::getline;
using std::ifstream;
using std::ios_base;
using std::regex;
using std::regex_match;

int main_app::createConfigFile() {
    ofstream ofs = ofstream(config::path_config.c_str(), ios_base::out);
    const char equ_sig = '=';
    const char midbracket_sign_st = '[';
    const char midbracket_sign_ed = ']';
    ofs << config::key_address << equ_sig << config::address << endl;
    ofs << config::key_port << equ_sig << config::port << endl;
    ofs << config::key_timeout_sec_conn << equ_sig << config::timeout_sec_conn << endl;
    ofs << "# default to stderr" << endl;
    ofs << config::key_path_logger << equ_sig << config::path_logger << endl;
    ofs << "# end with slash '/'" << endl;
    ofs << config::key_list_url2path_static << equ_sig << midbracket_sign_st << endl;
    for (auto &&p : config::list_url2path_static) {
        ofs << '\t' << p.first << equ_sig << p.second << endl;
    }
    ofs << midbracket_sign_ed << endl;
    ofs << "# end without slash '/', cgi file should be in a different dir with any staticly mapped path" << endl;
    ofs << config::key_list_url2file_cgi << equ_sig << midbracket_sign_st << endl;
    for (auto &&p : config::list_url2file_cgi) {
        ofs << '\t' << p.first << equ_sig << p.second << endl;
    }
    ofs << midbracket_sign_ed << endl;
    ofs << config::key_map_code2file_error << equ_sig << midbracket_sign_st << endl;
    for (auto &&p : config::map_code2file_error) {
        ofs << '\t' << p.first << equ_sig << p.second << endl;
    }
    ofs << midbracket_sign_ed << endl;
    ofs.close();
    return 0;
}

// -1 for error, -2 for help or create halox.conf
int main_app::parseArgs() {
    bool debug_tmp = config::debug;
    string address_tmp = config::address;
    string port_tmp = to_string(config::port);
    string log_level_tmp;
    for (auto &&p : logger::LIST_LOG_LV2STRING)
        if (p.first == config::log_level)
            log_level_tmp = p.second;
    string output_path_tmp = config::path_logger;

    const char *options = "hdca:p:l:o:";
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
        case 'c':
            createConfigFile();
            return -2;
        default:
            cerr << usage << endl;
            return -1;
        }
    }
    bool ok = utils::check_port(port_tmp) &&
              utils::check_addr(address_tmp) &&
              utils::check_log_level(log_level_tmp) &&
              utils::check_output_path(output_path_tmp);
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

// todo test and fix bugs, /f not mapped
int main_app::loadConfig() {
    string address_tmp = config::address;
    string port_tmp = to_string(config::port);
    string timeout_tmp = to_string(config::timeout_sec_conn);
    string path_logger_tmp = config::path_logger;

    typedef pair<string, string> PAIR_SS;
    int ret = 0;
    ifstream ifs = ifstream(config::path_config, ios_base::in);
    while (!ifs.eof()) {
        string line;
        getline(ifs, line);
        utils::trim(line);
        if (line.empty() || line[0] == '#') continue;
        PAIR_SS res_pair_ss;
        ret = utils::split(line, '=', res_pair_ss);
        if (ret == -1) {
            cerr << "error when parse config file, invalid config line" << endl;
            goto err_load_config;
        }
        string &first = res_pair_ss.first;
        string &second = res_pair_ss.second;
        if (first == config::key_address) {
            address_tmp = second;
        } else if (first == config::key_port) {
            port_tmp = second;
        } else if (first == config::key_timeout_sec_conn) {
            timeout_tmp = second;
        } else if (first == config::key_path_logger) {
            path_logger_tmp = second;
        } else if (second == "[" &&
                   (first == config::key_list_url2path_static ||
                    first == config::key_list_url2file_cgi ||
                    first == config::key_map_code2file_error)) {
            bool bracket_end_got = false;
            // clear and use user's
            if (first == config::key_list_url2path_static)
                config::list_url2path_static.clear();
            if (first == config::key_map_code2file_error)
                config::map_code2file_error.clear();
            while (!ifs.eof()) {
                string line2;
                getline(ifs, line2);
                utils::trim(line2);
                if (line2.empty()) continue;
                if (line2 == "]") {
                    bracket_end_got = true;
                    break;
                }
                PAIR_SS res2_pair_ss;
                ret = utils::split(line2, '=', res2_pair_ss);
                if (ret == -1) {
                    cerr << "error when parse config file, invalid path map line" << endl;
                    goto err_load_config;
                }
                if (first == config::key_list_url2path_static) {
                    // todo add '/' at end of string
                    config::list_url2path_static.push_back(res2_pair_ss);
                } else if (first == config::key_list_url2file_cgi) {
                    config::list_url2file_cgi.push_back(res2_pair_ss);
                } else if (first == config::key_map_code2file_error) {
                    if (utils::set_errorcode.find(res2_pair_ss.first) == utils::set_errorcode.end()) {
                        cerr << "error when parse config file, invalid error code" << endl;
                        goto err_load_config;
                    }
                    config::map_code2file_error[stoi(res2_pair_ss.first)] = res2_pair_ss.second;
                }
            }
            if (bracket_end_got == false) {
                cerr << "error when parse config file, end of array not found" << endl;
                goto err_load_config;
            }
        } else {
            cerr << "error when parse config file, unknown config line" << endl;
            goto err_load_config;
        }
    }
    bool ok;
    ok = utils::check_addr(address_tmp) &&
         utils::check_port(port_tmp) &&
         utils::check_output_path(path_logger_tmp) &&
         utils::check_timeout(timeout_tmp);
    if (ok == false) {
        cerr << "error when parse config file, invalid value format" << endl;
        goto err_load_config;
    }
    config::address = address_tmp;
    config::port = stoi(port_tmp);
    config::timeout_sec_conn = stod(timeout_tmp);
    config::path_logger = path_logger_tmp;
    if (config::debug) {
        cerr << "--------------debug--------------" << endl;
        cerr << config::address << ":" << config::port << endl;
        cerr << "timeout: " << config::timeout_sec_conn << endl;
        cerr << "log file: " << config::path_logger << endl;
        cerr << "static:" << endl;
        for (auto &&p : config::list_url2path_static) {
            cerr << "\t" << p.first << "=" << p.second << endl;
        }
        cerr << "cgi:" << endl;
        for (auto &&p : config::list_url2file_cgi) {
            cerr << "\t" << p.first << "=" << p.second << endl;
        }
        cerr << "error file:" << endl;
        for (auto &&p : config::map_code2file_error) {
            cerr << "\t" << p.first << "=" << p.second << endl;
        }
        cerr << "--------------debug--------------" << endl;
    }
    ifs.close();
    return 0;
err_load_config:
    ifs.close();
    return -1;
}
void main_app::init() {
    app_state = state::initialize;
    { // handle sigint
        struct sigaction sa;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = 0;
        sa.sa_handler = [](int) -> void { stop(); };
        if (sigaction(SIGINT, &sa, nullptr) == -1) {
            throw std::runtime_error("main_app.init: call to sigaction failed, exiting");
        }
    }
    { // config
        typedef pair<int, string> PAIR_IS;
        typedef pair<string, string> PAIR_SS;

        config::address = "127.0.0.1";
        config::port = 8686;
        config::backlog = 1024;
        config::timeout_sec_conn = 10;
        config::path_logger = config::path_default_logger_cerr;
        config::list_url2path_static = {PAIR_SS("/", "./")};
        config::map_code2file_error = {
            // PAIR_IS(response_header::CODE_NOT_FOUND, "./error.html"),
            // PAIR_IS(response_header::CODE_INTERNAL_SERVER_ERROR, "./error.html"),
        };

        config::debug = false;
        config::log_level = logger::LOG_LV_INFO;
    }
    { // load config
        if (access(config::path_config.c_str(), F_OK | R_OK) != -1) {
            // config file existence, load
            int ret = loadConfig();
            if (ret == -1) {
                throw std::runtime_error("load config failed");
            }
        } else {
            // create it
            createConfigFile();
        }
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
Usage: halox [-h] [-d] [-c] [-a addr] [-p port] [-l loglevel] [-o logfile]\n\n\
Options:\n\
    -d\t\t: print debug message\n\
    -c\t\t: write default configuration to halox.conf\n\
    -a addr\t: set address to listen on, default 127.0.0.1\n\
    -p port\t: set port to listen on, default 8686\n\
    -l loglevel\t: set log level, 'info'(default) or 'verbose'\n\
    -o logfile\t: set log file path, default stderr\n";

#endif // MAIN_APP_CPP
