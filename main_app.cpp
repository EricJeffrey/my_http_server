#if !defined(MAIN_APP_CPP)
#define MAIN_APP_CPP
#include "main_app.h"
#include "logger.h"
#include "main_listener.h"
#include <signal.h>

void main_app::init() {
    app_state = state::initialize;
    // handle sigint
    struct sigaction sa;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sa.sa_handler = [](int) -> void { stop(); };
    if (sigaction(SIGINT, &sa, nullptr) == -1) {
        logger::fail({"in ", __func__, " call to sigaction failed"}, true);
        return;
    }
    { // config
        config::address = "0.0.0.0";
        config::port = 8686;
        config::backlog = 1024;
        config::path_url_error = "static/error.html";

        config::path_file_error = "static/error.html";

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

        config::env_query_string_key = "query";
        config::path_logger = "cerr";
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
    logger::stop();
    for (auto &&pid : set_child_pids)
        kill(pid, SIGINT);
    exit(0);
}

state main_app::app_state;
set<pid_t> main_app::set_child_pids;

#endif // MAIN_APP_CPP
