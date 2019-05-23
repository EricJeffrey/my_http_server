#include "header_file/req_resp_header.h"
#include "header_file/svr_mgr.h"

// exit on error. err: error number, s: info, thread_exit: exit only this thread, sd: sd to close (only when thread_exit is false)
int err_exit(int err, const char *s, bool thread_exit, int sd) {
    printf("error: %d! %s.\n", err, s);
    if (thread_exit) {
        pthread_exit(0);
    } else {
        if (sd != -1)
            close(sd);
        exit(0);
    }
}

int main(int argc, char const *argv[]) {
    start_listen();
    return 0;
}

// request example
// GET /index.html HTTP/1.1
// Host: 127.0.0.1:2500
// User-Agent: Mozilla/5.0 (X11; Ubuntu; Linux x86_64; rv:61.0) Gecko/20100101 Firefox/61.0
// Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8
// Accept-Language: en-GB,en;q=0.5
// Accept-Encoding: gzip, deflate
// Connection: keep-alive
// Upgrade-Insecure-Requests: 1
// empty line
