#include "main_listener.h"
#include <execinfo.h>
#include <signal.h>

void init() {
    { // config
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

        config::timeout_sec_conn = 3;

        config::env_query_string_key = "query";
    }
    { // response phrase
        response_header::code2phrase[response_header::CODE_OK] = "OK";
        response_header::code2phrase[response_header::CODE_NOT_FOUND] = "Not Found";
        response_header::code2phrase[response_header::CODE_INTERNAL_SERVER_ERROR] = "Internal Server Error";

        response_header::STR_VERSION_HTTP_1_1 = "HTTP/1.1";
    }
}

// connection per thread model
int main(int argc, char const *argv[]) {
    init();
    main_listener listener;
    listener.start();
    return 0;
}
