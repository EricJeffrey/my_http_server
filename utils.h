#if !defined(UTILS_H)
#define UTILS_H

#include "config.h"

class utils {
private:
public:
    utils() {}
    ~utils() {}
    // 解析url路径与参数，返回路径类型
    // 1-static，0-cgi，-1-error
    static int parse_url(string url, string &path, vector<string> &paras_list) {
        path.clear();
        paras_list.clear();
        const size_t npos = string::npos;
        const size_t sz_url = url.size();
        if (sz_url == 0) return -1;
        size_t pos_of_question_sign = url.find_first_of('?');
        int res = -1;

        size_t st_path = 0;
        size_t ed_path = (pos_of_question_sign == npos ? sz_url : pos_of_question_sign);
        // static or cgi
        int type_static = -1;
        string dir_static_mapped;
        string file_cgi_mapped;
        // static dir url
        for (auto &&url2path : config::list_url2path_static) {
            const string &url_key_tmp = url2path.first;
            const size_t sz_url_tmp = url_key_tmp.size();
            if (sz_url > sz_url_tmp && url.substr(0, sz_url_tmp) == url_key_tmp) {
                st_path = sz_url_tmp;
                dir_static_mapped = url2path.second;
                type_static = 1;
                break;
            }
        }
        if (type_static == -1) {
            // cgi file url
            for (auto &&url2file : config::list_url2file_cgi) {
                const string &url_key_tmp = url2file.first;
                const size_t sz_url_tmp = url_key_tmp.size();
                // e.g. url='/abcd', url2file='/abc'->'xxx'
                if (sz_url >= sz_url_tmp && url.substr(0, sz_url_tmp) == url_key_tmp &&
                    (sz_url == sz_url_tmp || url[sz_url_tmp] == '?')) {
                    file_cgi_mapped = url2file.second;
                    type_static = 0;
                    break;
                }
            }
        }

        res = type_static;
        if (res == -1) return -1;
        // /static/? or /static/
        if (type_static == 1 && st_path == ed_path) return -1;
        path = (type_static == 1 ? dir_static_mapped + url.substr(st_path, ed_path - st_path) : file_cgi_mapped);
        // no ? found in url
        if (pos_of_question_sign == npos || pos_of_question_sign == sz_url - 1) return res;
        size_t st_para = pos_of_question_sign + 1;
        while (true) {
            size_t pos_and_sign = url.find_first_of('&', st_para);
            // no more '&'
            if (pos_and_sign == string::npos || pos_and_sign == sz_url) {
                paras_list.push_back(url.substr(st_para, sz_url - st_para));
                break;
            } else {
                if (pos_and_sign > st_para)
                    paras_list.push_back(url.substr(st_para, pos_and_sign - st_para));
                st_para = pos_and_sign + 1;
            }
        }
        return res;
    }
};

#endif // UTILS_H
