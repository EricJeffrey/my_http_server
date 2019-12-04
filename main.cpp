#include "main_listener.h"
#include <execinfo.h>
#include <signal.h>

void init() {
    { // config
        config::port = 8686;
        config::backlog = 1024;
        config::path_url_error = "static/error.html";

        config::path_file_error = "static/error.html";
        config::path_file_cgi_process = "./cgi/main_cgi";

        config::debug = false;
        config::log_level = logger::LOG_LV_INFO;

        config::list_url2path_static = {
            PSS("/static/", "./static/"),
            PSS("/hello/", "./hello/"),
            PSS("/file/", "./file/"),
        };
        config::list_url2file_cgi = {
            PSS("/take", "./take.py"),
            PSS("/cgi/tell", "./tell.py"),
            PSS("/find", "./find.py"),
        };
        config::env_query_string = "query";
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
