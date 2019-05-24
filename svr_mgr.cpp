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

/*
wrapper for send
sd: socket, ver_co_phr: http/1.0 200 OK, vcp_len: len of ver_co_phr
h_body: general header content, msg_body: data, m_body_len: size of data
*/
int send_msg(int sd, con_c_str ver_co_phr, int vcp_len, vector<pss> *h_body, con_c_str msg_body, int mb_len) {
    int cont_len = 0;
    { // get content length
        // first line len
        if (ver_co_phr != NULL)
            cont_len += vcp_len + 2;
        // header len
        if (h_body != NULL)
            for (const pss ss : *h_body)
                // key: value\r\n
                cont_len += ss.first.size() + ss.second.size() + 2 + 2;
        // body len
        if (msg_body != NULL)
            cont_len += mb_len + 1;
    }
    char *buf = (char *)malloc(cont_len);
    const char spr_fail[] = "sprintf failed";
    int t = 0;

    // first line
    if ((t += sprintf(buf + t, "%s\r\n", ver_co_phr)) < 0)
        err_exit(t, spr_fail, 1, -1);

    // header
    if (h_body != NULL)
        for (const pss ss : *h_body)
            if ((t += sprintf(buf + t, "%s: %s\r\n", ss.first.c_str(), ss.second.c_str())) < 0)
                err_exit(t, spr_fail, 1, -1);

    // msg body
    if ((t += sprintf(buf + t, "\r\n%s", msg_body)) < 0)
        err_exit(t, spr_fail, 1, -1);

    // send and return
    if (send(sd, buf, cont_len - 1, 0) == -1)
        return -1;

    // free memory
    free(buf);
    return 0;
}

// handle client connectioin
void *handle_conn(void *x) {
    const int sd = *(int *)(x);
    const int buf_sz = 4096;
    char buf[buf_sz] = {};

    // recv request
    int err, len;
    while (1) {
        memset(buf, 0, buf_sz);
        (err = recv(sd, buf, buf_sz, 0)) == -1 ? err_exit(err, "recv failed", 1, -1) : len = err;

        // get header
        my_req_header header;
        parse_headers(buf, len, &header);
        // header.print_me();

        // get file from header.path
        if (header.path == "/")
            header.path = "/index.html";
        string file_path = PAGE_DIR_PATH + header.path;
        if (send_file(file_path, sd) == -1)
            err_exit(-1, "send_file failed", 1, -1);
    }
    close(sd);
}

// read fpath content and send
int send_file(string fpath, int sd) {
    // open file and check existence
    FILE *fp = fopen(fpath.c_str(), "r");
    if (fp == NULL) {
        // send 404 Message
        const char vcp[] = "HTTP/1.1 404 Not Found";
        if (send_msg(sd, vcp, sizeof(vcp), NULL, msg_body_404, sizeof(msg_body_404)) == -1)
            return -1;
        return 0;
    }

    // file size
    fseek(fp, 0L, SEEK_END);
    long f_sz = ftell(fp);
    ferror(fp) ? err_exit(f_sz, "ftell failed", 1, -1) : 0;
    rewind(fp);

    // construct msg header
    vector<pss> h_head;
    const char vcp[] = "HTTP/1.1 200 OK";
    h_head.push_back(pss("Content-length", std::to_string(f_sz)));
    bool first = 1;

    // read file data and send
    const int buf_sz = 2048;
    char buf[buf_sz] = {};

    // read data into buf
    while (feof(fp) != EOF) {
        int n = fread(buf, 1, buf_sz, fp), err = 0;
        if (err = ferror(fp))
            err_exit(err, "fread file failed", 1, -1);

        // send it as msg body
        if (n > 0) {
            if (first && send_msg(sd, vcp, sizeof(vcp), &h_head, buf, n) == -1)
                return -1;
            else if (send(sd, buf, n, 0) == -1)
                return -1;
        }
    }
    return 0;
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
        printf("socket descriptor: %d, connection established, now begin handle.\n", acc_sd);
        pthread_t sub_thr;
        // handle this conn
        (err = pthread_create(&sub_thr, NULL, handle_conn, (void *)&acc_sd)) != 0 ? err_exit(err, "create sub thread failed", 0, sd) : 0;
    }
    close(sd);
}
