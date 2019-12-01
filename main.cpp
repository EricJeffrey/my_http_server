#include "main_listener.h"
#include <execinfo.h>
#include <signal.h>

void handler(int sig) {
    void *array[10];
    size_t size;

    // get void*'s for all entries on the stack
    size = backtrace(array, 10);

    // print out all the frames to stderr
    fprintf(stderr, "Error: signal %d:\n", sig);
    backtrace_symbols_fd(array, size, STDERR_FILENO);
    exit(1);
}
void init() {
    { // config
        config::port = 8686;
        config::backlog = 1024;
        config::path_url_static = "static/";
        config::path_url_cgi = "cgi/";
        config::path_url_error = "static/error.html";

        config::path_dir_static_root = "./static/";
        config::path_dir_cgi_root = "./cgi/";
        config::path_file_cgi_process = "./cgi/main_cgi";

        config::debug = false;
        config::log_level = logger::LOG_LV_INFO;
    }
    { // response phrase
        response_header::code2phrase[response_header::CODE_OK] = "OK";
        response_header::code2phrase[response_header::CODE_NOT_FOUND] = "Not Found";
        response_header::code2phrase[response_header::CODE_INTERNAL_SERVER_ERROR] = "Internal Server Error";

        response_header::STR_VERSION_HTTP_1_1 = "HTTP/1.1";
    }
    // install signal handler
    signal(SIGSEGV, handler);
}

// connection per thread model
int main(int argc, char const *argv[]) {
    init();
    main_listener listener;
    listener.start();
    return 0;
}
