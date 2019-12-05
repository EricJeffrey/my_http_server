#include "include_heads.h"

#if !defined(REQ_RESP_HEADER)
#define REQ_RESP_HEADER
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

// response msg_body, include header and content
struct my_resp_msg {
    string version;
    int status_code;
    string phrase;
    vector<string> keys, values;
    int header_len;
    const char * msg_body;
    int msg_body_len;

    // init heaer
    my_resp_msg(const char *v, int code, const char *p) {
        version = v, status_code = code, phrase = p;
        header_len = version.size() + sizeof(int) + phrase.size() + 2 + 2 + 2; // two space, two \r\n
    }

    // set header content, 0 success, -1 on error
    int set_key_vals(const vector<string> &ks, const vector<string> &vals) {
        if (ks.size() != vals.size()) {
            printf("error: ks and vals size not equal.");
            return -1;
        }
        for (int i = 0; i < (int)ks.size() && i < (int)vals.size(); i++) {
            keys.push_back(ks[i]), values.push_back(vals[i]);
            header_len += ks[i].size() + values[i].size() + 3; // key:value\r\n
        }
        return 0;
    }

    // set response msg body and its lenth
    int set_msg_body(const char *mb, int mb_len) {
        msg_body = mb, msg_body_len = mb_len;
    }

    // convert response msg to c str (for send), return msg body len, -1 on error
    int get_msg_cstr(char *buf, int buf_len) {
        if (buf_len < msg_body_len + header_len) {
            printf("error get_msg_cstr: buf_len < msg_body_len.");
            return -1;
        }
        int t = 0;
        t += sprintf(buf, "%s %d %s\r\n", version.c_str(), status_code, phrase.c_str());
        for (int i = 0; i < (int)keys.size(); i++) {
            t += sprintf(buf + t, "%s:%s\r\n", keys[i].c_str(), values[i].c_str());
        }
        t += sprintf(buf + t, "\r\n%s", msg_body);
        return t;
    }
};

#endif // REQ_RESP_HEADER
