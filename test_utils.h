#include "utils.h"
#include <iomanip>
#include <iostream>
#include <vector>
using std::cin;
using std::cout;
using std::endl;
using std::vector;

void test_parse_url() {
    vector<string> url_list = {"", "cgi", "/stait", "/static", "/static/", "/cg", "/cgi/",
                               "/static/a", "/static/?", "/static/?wd=sdf", "/static/bcde",
                               "/static/cd?", "/static/cd?a", "/static/cd?bcdefgh&sd",
                               "/static/cd?wd=2&sd", "/static/cd?wd&sd=sdg"};
    int i = 0;
    for (auto &&url_tmp : url_list) {
        cout << i++ << '\t';
        cout << std::setw(23) << url_tmp << '\t' << std::setw(0);
        vector<string> paras;
        string path;
        int ret = utils::parse_url(url_tmp, path, paras);
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