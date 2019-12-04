#include "utils.h"
#include <iomanip>
#include <iostream>
#include <vector>
using std::cin;
using std::cout;
using std::endl;
using std::vector;

void test_parse_url() {
    vector<string> url_list = {
        "",
        "cgi",
        "/stait",
        "/static",
        "/hello",
        "/static/",
        "/cg",
        "/fil",
        "/cgi/",
        "/static/a",
        "/static/?",
        "/static/?wd=sdf",
        "/static/bcde",
        "/static/cd?",
        "/static/cd?a",
        "/static/cd?bcdefgh&sd",
        "/static/cd?wd=2&sd",
        "/static/cd?wd&sd=sdg",
        "/tak",
        "ta",
        "/tak?a=2",
        "/take?a=2&b=3",
        "/take?a&b",
        "/find?&b",
        "/find?a&cx=34"};
    int i = 0;
    for (auto &&url_tmp : url_list) {
        cout << i++ << '\t';
        cout << std::setw(23) << url_tmp << '\t' << std::setw(0);
        vector<string> paras;
        string path;
        int ret = utils::parseUrl(url_tmp, path, paras);
        if (ret == -1) {
            cout << "failed" << endl;
        } else {
            const string comma = ",\t";
            cout << std::setw(6) << path << comma << std::setw(0);
            for (auto &&para_tmp : paras)
                cout << para_tmp << comma;
            cout << endl;
        }
    }
}