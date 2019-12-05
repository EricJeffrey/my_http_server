#include "header_file/svr_mgr.h"

void parse_headers(const char *buf, int len, my_req_header *header) {
    printf("in parse_headers...\n");
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
        if (j == 0)
            header->method.push_back(ch);
        else
            (j == 1 ? header->path.push_back(ch) : header->version.push_back(ch));
    }

    // parse keys and values
    bool k;
    for (i += 2, j = k = 0; i < len; i++) {
        const char ch = buf[i];
        if (ch == '\r') {
            i++, j++;
            k = 0;
            continue;
        } else if (k == 0 && ch == ':') {
            k = 1;
            continue;
        }
        k == 0 ? header->keys[j].push_back(ch) : header->values[j].push_back(ch);
    }
}

// wrapper for recv
void recv_msg() {}

int send_msg(int sd, con_c_str fir_line, int fir_line_len, vector<pss> *gen_head, con_c_str msg_body, int mb_len) {
    printf("in send_msg...\n");
    int cont_len = 0;
    { // get content length
        // first line len
        if (fir_line != NULL)
            cont_len += fir_line_len + 2;
        // header len
        if (gen_head != NULL)
            for (const pss ss : *gen_head)
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
    if ((t += sprintf(buf + t, "%s\r\n", fir_line)) < 0)
        err_exit(t, spr_fail, 1, sd);

    // header
    if (gen_head != NULL)
        for (const pss ss : *gen_head)
            if ((t += sprintf(buf + t, "%s: %s\r\n", ss.first.c_str(), ss.second.c_str())) < 0)
                err_exit(t, spr_fail, 1, sd);

    // msg body
    if ((t += sprintf(buf + t, "\r\n%s", msg_body)) < 0)
        err_exit(t, spr_fail, 1, sd);

    // send and return
    if (send(sd, buf, cont_len - 1, 0) == -1)
        return -1;

    // free memory
    // free(buf); // this will cause client cannot read data

    printf("exiting send_msg.\n");
    return 0;
}

// read fpath content and send
int send_file(string fpath, int sd) {
    printf("in send_file... fpath: %s, sd: %d\n", fpath.c_str(), sd);
    // open file and check existence
    FILE *fp = fopen(fpath.c_str(), "r");
    if (fp == NULL) {
        // send 404 Message
        const char fir_line[] = "HTTP/1.1 404 Not Found";
        vector<pss> gen_head = {pss("Content-length", "23")};
        if (send_msg(sd, fir_line, sizeof(fir_line), &gen_head, msg_body_404, sizeof(msg_body_404)) == -1)
            return -1;
        return 0;
    }

    // file size
    fseek(fp, 0L, SEEK_END);
    long f_sz = ftell(fp);
    rewind(fp);
    ferror(fp) ? err_exit(f_sz, "ftell failed", 1, sd) : 0;

    // construct msg header
    const char fir_line[] = "HTTP/1.1 200 OK";
    vector<pss> gen_head = {
        pss("Content-length", std::to_string(f_sz)),
        pss("Connection", "keep-alive"),
    };

    // read file data and send
    const int buf_sz = 4095, buf_sz_hex_bytes = 3;
    const int buf_data_sz = buf_sz - buf_sz_hex_bytes - 2 - 2;
    int err = 0;
    char buf[buf_sz] = {};

    // no need chunk
    if (f_sz <= buf_sz) {
        int n = fread(buf, 1, buf_sz, fp);
        if ((err = ferror(fp)) == 0 && n >= 0) {
            printf("n: %d\n", n);
            err = send_msg(sd, fir_line, sizeof(fir_line), &gen_head, buf, n);
            fclose(fp);
            return err;
        } else {
            err_exit(err, "ferror or fread failed", 1, sd);
        }
    }

    /*
    chunk need, format:
    7\r\nMozilla\r\n =_= 9\r\nDeveloper\r\n =_= 7\r\nNetwork\r\n =_= 0\r\n\r\n
    */
    for (bool first = 1; feof(fp) != EOF; first = 0) {
        memset(buf, 0, buf_sz);

        // write buf length and \r\n
        int n, t;
        n = fread(buf + buf_sz_hex_bytes + 2, 1, buf_data_sz, fp);
        if ((err = ferror(fp)) || n < 0)
            err_exit(err, "fread file failed or n < 0", 1, sd);
        char *buf_d = buf;
        buf_d += (n >= 0x100 ? 0 : n >= 0x10 ? 1 : 2);
        t = sprintf(buf_d, "%x\r\n", n);

        // write end \r\n
        *(buf_d + buf_sz_hex_bytes + 2 + n) = '\r';
        *(buf_d + buf_sz_hex_bytes + 2 + n + 1) = '\n';

        // send it as msg body
        if (first)
            err = send_msg(sd, fir_line, sizeof(fir_line), &gen_head, buf_d, n);
        else
            err = send(sd, buf_d, n, 0);
        if (err == -1)
            return -1;
    }
    sprintf(buf, "0\r\n\r\n");
    if (send(sd, buf, 4, 0) == -1)
        return -1;
    return 0;
}

// handle client connectioin
void *handle_conn(void *x) {
    printf("in handle_conn...\n");
    const int sd = *(int *)(x);
    const int buf_sz = 1024;
    char buf[buf_sz] = {};

    // recv request
    int ret, len;
    for (int i = 0; i < 15; i++) {
        memset(buf, 0, buf_sz);
        ret = recv(sd, buf, buf_sz, 0);
        ret == -1 ? err_exit(ret, "recv failed", 1, sd) : len = ret;
        printf("request received, parsing header...\n");

        // get header
        my_req_header header;
        parse_headers(buf, len, &header);
        // header.print_me();

        // get file from header.path
        if (header.path == "/")
            header.path = "/index.html";
        string file_path = PAGE_DIR_PATH + header.path;
        if (send_file(file_path, sd) == -1)
            err_exit(-1, "send_file failed", 1, sd);
    }
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
        // printf("socket descriptor: %d, connection established, now begin handle.\n", acc_sd);
        pthread_t sub_thr;
        // handle this conn
        err = pthread_create(&sub_thr, NULL, handle_conn, (void *)&acc_sd);
        err != 0 ? err_exit(err, "create sub thread failed", 0, sd) : 0;
    }
    close(sd);
}
