#include <arpa/inet.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <pthread.h>
#include <string>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>

using std::string, std::vector;

#define SVR_PORT 2500
#define BACKLOG 20

int err_exit(int err, const char *s) {
    printf("error: %d! %s.\n", err, s);
    exit(0);
}

/*
Http data packet parse, url path parse.
1. recv a http and print its value
    1. change url path and see the result
2. send a response to clients
3. map all HTML file in DIR
*/
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

// headers store message
struct my_header {
    string path, method, proto;
    vector<string> keys, values;

    void print_me() {
        printf("path: %s\nmethod: %s\nproto: %s\n", path.c_str(), method.c_str(), proto.c_str());

        for (int i = 0; i < keys.size(); i++)
            printf("%s:%s\n", keys[i].c_str(), values[i].c_str());
    }
};

// parse http headers, saved in header
void parse_headers(const char *buf, int len, my_header *header) {
    if (header == NULL)
        return;
    int i, j;

    // init keys & values
    for (i = j = 0; i < len; i++)
        buf[i] == '\r' ? j++ : 0;
    header->keys.resize(j), header->values.resize(j);

    // parse methods path proto
    for (i = j = 0; i < len && buf[i] != '\r'; i++) {
        const char ch = buf[i];
        if (ch == ' ') {
            ++j;
            continue;
        }
        if (j == 0)
            header->method.push_back(ch);
        else if (j == 1)
            header->path.push_back(ch);
        else
            header->proto.push_back(ch);
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
void handle_conn(int sd) {
    const int buf_sz = 2048;
    char buf[buf_sz] = {};

    // recv request
    int err, len;
    (err = recv(sd, buf, buf_sz, 0)) == -1 ? err_exit(err, "recv failed") : len = err;

    // get header
    my_header header;
    parse_headers(buf, len, &header);
    header.print_me();

    // get file from header.path
    // read file content

    // send response
    const char resp[] = "HTTP/1.1 200 OK\r\nContent-Length: 11\r\n\r\nHelloWorld\n";
    (len = send(sd, resp, sizeof(resp), 0)) == -1 ? err_exit(len, "send failed") : 0;
}

// server listen
void start_listen() {
    // create addr
    sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET, addr.sin_port = htons(SVR_PORT), addr.sin_addr.s_addr = htonl(INADDR_ANY);

    // create socket
    int err, sd;
    (err = socket(AF_INET, SOCK_STREAM, 0)) == -1 ? err_exit(sd, "create socket failed") : sd = err;

    // bind to addr
    (err = bind(sd, (sockaddr *)&addr, sizeof(addr))) != 0 ? err_exit(err, "bind socket failed") : 0;

    // listen
    (err = listen(sd, BACKLOG)) != 0 ? err_exit(err, "listen failed") : 0;
    printf("server listenning now...\n");

    // accept, wait for conn
    int acc_sd;
    (err = accept(sd, NULL, NULL)) == -1 ? err_exit(acc_sd, "accept failed") : acc_sd = err;
    printf("connection established.\n");

    // handle this conn
    handle_conn(acc_sd);

    // recv a packet from client
    // const int buf_sz = 1024;
    // char buf[buf_sz] = {};
    // int len = recv(acc_sd, buf, buf_sz, 0);
    // for (int i = 0; i < len; i++) {
    //     const char ch = buf[i];
    //     printf(((ch >= 32 && ch < 128) || ch == 10 || ch == 13) ? "%c" : "0x%x", ch);
    // }
    // printf("\n");

    // // send a response to client - HelloWorld
    // const char resp[] = "HTTP/1.1 200 OK\r\nContent-Length: 11\r\n\r\nHelloWorld\n";
    // (len = send(acc_sd, resp, sizeof(resp), 0)) == -1 ? err_exit(len, "send failed") : 0;
}

int main(int argc, char const *argv[]) {
    // mix cin and cout
    // std::ios::sync_with_stdio(1);
    start_listen();
    return 0;
}
