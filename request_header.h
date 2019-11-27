#if !defined(REQUEST_HEADER_H)
#define REQUEST_HEADER_H

#include "utils.h"
#include <map>
#include <vector>

using std::map;
using std::pair;
using std::vector;
typedef pair<string, string> PAIR_SS;

class request_header {
public:
    string version;
    string method;
    string url;
    map<string, string> mp_gene_req_headers;
    vector<PAIR_SS> list_pair_headers;

    request_header() {}
    ~request_header() {}

    bool has(string key) {
        return mp_gene_req_headers.find(key) != mp_gene_req_headers.end();
    }
    // ensure key exists!
    string get(string key) {
        return mp_gene_req_headers[key];
    }

    // -1 for error
    static int fromHeaderLines(vector<string> line_headers, request_header &header) {
        logger::verbose({"creating request header from header lines"});
        if (line_headers.size() <= 0) {
            logger::fail({__func__, "failed, line_headers.size = 0"});
            return -1;
        };

        string req_line = line_headers[0];
        int pos_start = 0;
        int pos_end = 0;
        const char space = ' ';
        pos_end = req_line.find(space, pos_start);
        header.method = req_line.substr(pos_start, pos_end - pos_start);
        pos_start = pos_end + 1;
        pos_end = req_line.find(space, pos_start);
        header.url = req_line.substr(pos_start, pos_end - pos_start);
        pos_start = pos_end + 1;
        pos_end = req_line.find(space, pos_start);
        header.version = req_line.substr(pos_start, pos_end - pos_start);
        logger::debug({"head.url: ", header.url, " header.method: ", header.method});
        const char colon = ':';
        for (size_t i = 1; i < line_headers.size(); i++) {
            string line_tmp = line_headers[i];
            pos_end = line_tmp.find(colon);

            string key = line_tmp.substr(0, pos_end);
            pos_end += 1;
            string val = line_tmp.substr(pos_end, line_tmp.size() - pos_end);
            header.mp_gene_req_headers[key] = val;
            header.list_pair_headers.push_back(PAIR_SS(key, val));
        }
        return 0;
    }
};

#endif // REQUEST_HEADER_H
