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

// response header
struct my_resp_header {
    string version;
    int status_code;
    string phrase;
    vector<string> keys, values;
};

#endif // REQ_RESP_HEADER
