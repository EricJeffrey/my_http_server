#if !defined(UTILS_H)
#define UTILS_H

#include "config.h"

class utils {
private:
public:
    utils() {}
    ~utils() {}
    /**
     * 解析请求方法、路径与参数
     * - url路径 '/static/', '/cgi/'
     * - url参数 /static/index.html?a=2, 参数存储为字符串（包括或不包括 等号 ）
     * -1 for error
     * */
    static int parse_url(string url, string &path, vector<string> &paras_list) {
        path.clear();
        paras_list.clear();
        const size_t npos = string::npos;
        const size_t sz_url = url.size();
        size_t pos_of_question_sign = url.find_first_of('?');

        size_t st_path = 0;
        size_t ed_path = (pos_of_question_sign == npos ? sz_url : pos_of_question_sign);
        // static or cgi
        if (url.find(config::path_url_static) == 0) {
            st_path = config::path_url_static.size();
            path = config::path_dir_static_root;
        } else if (url.find(config::path_url_cgi) == 0) {
            st_path = config::path_url_cgi.size();
            path = config::path_dir_cgi_root;
        } else { // error
            return -1;
        }
        if (st_path == ed_path) return -1;
        // root + relative_path
        path += url.substr(st_path, ed_path - st_path);
        // no ? found in url
        if (pos_of_question_sign == npos) return 0;
        size_t st_para = pos_of_question_sign + 1;
        while (true) {
            size_t pos_and_sign = url.find_first_of('&', st_para);
            // no more '&'
            if (pos_and_sign == string::npos || pos_and_sign == sz_url) {
                paras_list.push_back(url.substr(st_para, sz_url - st_path));
                break;
            } else {
                paras_list.push_back(url.substr(st_para, pos_and_sign - st_para));
                st_para = pos_and_sign + 1;
            }
        }
        return 0;
    }
};

#endif // UTILS_H
