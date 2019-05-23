#include "header_file/svr_mgr.h"

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
