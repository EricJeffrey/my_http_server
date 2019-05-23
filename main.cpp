#include <arpa/inet.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <pthread.h>
#include <string>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

using std::string, std::vector;

#define SVR_PORT 2500
#define BACKLOG 20
#define PAGE_DIR_PATH "./pages"

/*
exit on error. 
err: error number  s: info  
thread_exit: exit only this thread  sd: sd to close (only when thread_exit is false)
*/
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

/*
Http data packet parse, url path parse.
1. recv a http and print its value
    1. change url path and see the result
2. send a response to clients
3. map all HTML file in DIR
*/

// request headers
struct my_req_header {
    string path, method, version;
    vector<string> keys, values;

    void print_me() {
        printf("path: %s\nmethod: %s\nproto: %s\n", path.c_str(), method.c_str(), version.c_str());

        for (int i = 0; i < keys.size(); i++)
            printf("%s:%s\n", keys[i].c_str(), values[i].c_str());
    }
};

// response header
struct my_resp_header {
    string version;
    int status_code;
    string phrase;
    vector<string> keys, values;
};

// parse http headers, saved in header
void parse_headers(const char *buf, int len, my_req_header *header) {
    if (header == NULL)
        return;

    int i, j;
    // init keys & values
    for (i = j = 0; i + 3 < len; i++) {
        if (buf[i] == '\r' && buf[i + 1] == '\n' && buf[i + 2] == '\r' && buf[i + 3] == '\n')
            break;
        buf[i] == '\r' && buf[i + 1] == '\n' ? j++ : 0;
    }
    header->keys.resize(j), header->values.resize(j);

    // parse methods path proto
    for (i = j = 0; i < len && buf[i] != '\r'; i++) {
        const char ch = buf[i];
        if (ch == ' ') {
            ++j;
            continue;
        }
        j == 0 ? header->method.push_back(ch) : (j == 1 ? header->path.push_back(ch) : header->version.push_back(ch));
    }

    // parse keys and values
    bool k;
    for (i += 2, j = k = 0; i < len; i++) {
        const char ch = buf[i];
        if (ch == '\r') {
            i++, j++;
            k = 0;
            continue;
        } else if (ch == ':') {
            k = 1;
            continue;
        }
        k == 0 ? header->keys[j].push_back(ch) : header->values[j].push_back(ch);
    }
}

// wrapper for recv
void recv_msg() {}

// wrapper for send
void send_msg() {}

// handle client connectioin
void *handle_conn(void *x) {
    const int sd = *(int *)(x);
    const int buf_sz = 4096;
    char buf[buf_sz] = {};

    // recv request
    int err, len;
    (err = recv(sd, buf, buf_sz, 0)) == -1 ? err_exit(err, "recv failed", 1, -1) : len = err;

    // get header
    my_req_header header;
    parse_headers(buf, len, &header);
    // header.print_me();

    // get file from header.path
    if (header.path == "/")
        header.path = "/index.html";
    string file_path = PAGE_DIR_PATH + header.path;
    struct stat stat_buf;
    if (err = stat(file_path.c_str(), &stat_buf) != 0) {
        // send 404
        const char resp[] = "HTTP/1.1 404 NOT_FOUND\r\nContent-Length: 11\r\n\r\nHelloWorld\n";
        (len = send(sd, resp, sizeof(resp), 0)) == -1 ? err_exit(len, "send failed", 1, -1) : 0;
    } else {
        // read file content
        FILE *fp = fopen(file_path.c_str(), "r");
        memset(buf, 0, sizeof(buf));
        fread(buf, 1, buf_sz, fp);
        char resp[buf_sz + 200] = {};
        int sz;
        while (buf[sz] != '\0')
            sz++;
        sprintf(resp, "HTTP/1.1 200 OK\r\nContent-Length: %d\r\n\r\n%s", sz, buf);
        // printf("file content: %s\n", buf);
        (len = send(sd, resp, sizeof(resp), 0)) == -1 ? err_exit(len, "send failed", 1, -1) : 0;
    }
    shutdown(sd, SHUT_RDWR);
    close(sd);
}

// server listen
void start_listen() {
    // create addr
    sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET, addr.sin_port = htons(SVR_PORT), addr.sin_addr.s_addr = htonl(INADDR_ANY);

    // create socket
    int err, sd;
    (err = socket(AF_INET, SOCK_STREAM, 0)) == -1 ? err_exit(sd, "create socket failed", 0, -1) : sd = err;

    // bind to addr
    (err = bind(sd, (sockaddr *)&addr, sizeof(addr))) != 0 ? err_exit(err, "bind socket failed", 0, sd) : 0;

    // listen
    (err = listen(sd, BACKLOG)) != 0 ? err_exit(err, "listen failed", 0, sd) : 0;
    printf("server listenning now...\n");

    // accept, wait for conn
    while (1) {
        int acc_sd;

        // wait user interact
        printf("1 -> accept, 0 -> exit: ");
        scanf("%d", &acc_sd);
        if (acc_sd == 0)
            break;

        (err = accept(sd, NULL, NULL)) == -1 ? err_exit(acc_sd, "accept failed", 0, sd) : acc_sd = err;
        printf("connection established, now begin handle.\n");
        pthread_t sub_thr;
        // handle this conn
        (err = pthread_create(&sub_thr, NULL, handle_conn, (void *)&acc_sd)) != 0 ? err_exit(err, "create sub thread failed", 0, sd) : 0;
    }
    close(sd);
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
