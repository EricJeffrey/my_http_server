#if !defined(UTILS_H)
#define UTILS_H

#include "config.h"
#include "logger.h"
#include <fcntl.h>
#include <regex>
#include <string>
#include <sys/stat.h>

using std::regex;
using std::regex_match;

using std::to_string;

class utils {
private:
public:
    static const set<string> set_errorcode;

    utils() {}
    ~utils() {}

    static bool check_timeout(string timeout) {
        return regex_match(timeout, regex("[1-9]\\d*.\\d*|0.\\d*[1-9]\\d*"));
    }
    static bool check_port(string port) {
        return regex_match(port, regex("[1-9]{1}[0-9]{3,4}"));
    };
    static bool check_addr(string addr) {
        return regex_match(addr, regex("(25[0-5]|2[0-4]\\d|[0-1]\\d{2}|[1-9]?\\d).(25[0-5]|2[0-4]\\d|[0-1]\\d{2}|[1-9]?\\d).(25[0-5]|2[0-4]\\d|[0-1]\\d{2}|[1-9]?\\d).(25[0-5]|2[0-4]\\d|[0-1]\\d{2}|[1-9]?\\d)"));
    };
    static bool check_log_level(string log_level) {
        for (auto &&p : logger::LIST_LOG_LV2STRING)
            if (p.second == log_level) return true;
        return false;
    };
    // split string with [ch], -1 for error
    static int split(const string &s, const char ch, PAIR_SS &res) {
        size_t pos = 0;
        pos = s.find('=');
        if (pos == 0 || pos == s.size() || pos == string::npos) return -1;
        res.first = s.substr(0, pos);
        pos += 1;
        res.second = s.substr(pos, s.size() - pos);
        return 0;
    };
    // trim [s] in place, only whitespace
    static void trim(string &s) {
        size_t st = 0;
        size_t ed = s.size();
        while (st < s.size() && s[st] == ' ') st++;
        while (ed >= 0 && s[ed] == ' ') ed--;
        if (st <= ed) s = s.substr(st, ed - st + 1);
    };
    // check existence of [path], create it and open it
    static bool check_output_path(string path) {
        if (path == config::path_default_logger_cerr || access(path.c_str(), F_OK | W_OK) != -1) return true;
        int fd = 0;
        if (creat(path.c_str(), S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP) == -1) {
            if ((fd = open(path.c_str(), O_RDWR | O_TRUNC)) == -1)
                return false;
            close(fd);
        }
        return true;
    };
    // 解析url路径与参数，返回路径类型
    // 1-static，0-cgi，-1-error
    static int parseUrl(string url, string &path, vector<string> &paras_list) {
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

    // 判断文件是否为普通文件，-1 error, 0 not, 1 ok
    static int isRegFile(const char *path) {
        struct stat path_info;
        int ret = stat(path, &path_info);
        if (ret == -1) {
            logger::info({"stat file failed"});
            return -1;
        }
        if (path_info.st_mode & S_IFREG) return 1;
        return 0;
    };
    // 从 fd 读取数据直到 [read] 返回 0
    // return number of bytes read, -1 for error, -2 for EAGAIN|EWOULDBLOCK
    static int readAll(int fd, string &res) {
        res.clear();
        int ret = 0;
        const size_t sz_buf = 2048;
        size_t n;
        char buffer[sz_buf];
        while (true) {
            // will block on pipe
            ret = read(fd, buffer, sz_buf);
            int errno_tmp = errno;
            if (ret == -1) {
                if (errno_tmp == EINTR)
                    continue;
                if (errno_tmp == EAGAIN || errno_tmp == EWOULDBLOCK) {
                    logger::info({"read on sd: ", to_string(fd), " would block"});
                    return -2;
                }
                logger::fail({"in ", __func__, ": call to read failed"}, true);
                cerr << "read failed" << endl;
                return -1;
            }
            if (ret == 0) break;
            n += ret;
            res += string(buffer, ret);
        }
        return n;
    };

    // 将 [s] 中的内容写到 fd 中，-1 for error
    static int writeStr2Fd(const string &s, int fd) {
        const char *buf = s.c_str();
        int sz = s.size();
        while (true) {
            int ret = write(fd, buf, sz);
            // all bytes written
            if (ret == sz) return 0;
            if (ret == -1) {
                logger::fail({"in ", __func__, ": failed with string(first 20 bytes): ", s.substr(0, std::max((int)s.size(), 20))}, true);
                return -1;
            }
            if (ret < sz) {
                // interrupted by signal
                int errno_tmp = errno;
                if (errno_tmp == EINTR) {
                    buf += ret;
                    sz -= ret;
                    continue;
                }
                logger::fail({"in ", __func__, ": failed, bytes written less than size(ret < s.size())"}, true);
                return -1;
            }
        }
    }
};

const set<string> utils::set_errorcode = {"400", "401", "402", "403", "404", "405", "406",
                                          "407", "408", "409", "410", "411", "412", "413",
                                          "414", "415", "416", "417", "500", "501", "502",
                                          "503", "504", "505"};

#endif // UTILS_H
