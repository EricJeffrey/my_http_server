#if !defined(RESPONSE_HEADER_H)
#define RESPONSE_HEADER_H

#include "utils.h"
#include <map>
#include <string>
#include <sys/stat.h>

using std::map;
using std::string;
using std::to_string;

typedef std::pair<int, string> P_IS;

class response_header {
public:
    static map<int, string> code2phrase;
    static string STR_VERSION_HTTP_1_1;

    static const int CODE_INTERNAL_SERVER_ERROR;
    static const int CODE_NOT_FOUND;
    static const int CODE_OK;

    string version;
    int status_code;
    string phrase;
    map<string, string> mp_gene_headers;

    response_header() {}
    ~response_header() {}

    // header with content-type:html, content-length:size of file of [path]
    static int htmlHeader(string path_file, response_header &header, int status_code = 200) {
        int ret = 0;
        struct stat file_info;
        ret = stat(path_file.c_str(), &file_info);
        if (ret == -1) {
            logger::fail({__func__, "stat file: ", path_file, " failed"}, true);
            return -1;
        }
        header.version = STR_VERSION_HTTP_1_1;
        header.status_code = status_code;
        header.phrase = code2phrase[status_code];
        header.mp_gene_headers["Content-Type"] = "text/html;charset=UTF-8";
        header.mp_gene_headers["Content-Length"] = to_string(file_info.st_size);
        return 0;
    }
    string toString() {
        stringstream ss;
        const char sp = ' ';
        const string crlf = "\r\n";
        const char colon = ':';
        ss << version << sp << status_code << sp << phrase << crlf;
        for (auto &&p : mp_gene_headers)
            ss << p.first << colon << p.second << crlf;
        ss << crlf;
        return ss.str();
    }
};

const int response_header::CODE_OK = 200;
const int response_header::CODE_NOT_FOUND = 404;
const int response_header::CODE_INTERNAL_SERVER_ERROR = 500;

string response_header::STR_VERSION_HTTP_1_1 = "HTTP/1.1";
map<int, string> response_header::code2phrase;
#endif // RESPONSE_HEADER_H
