#if !defined(CONFIG_H)
#define CONFIG_H
#include <string>
#include <vector>
using std::pair;
using std::string;
using std::vector;

typedef pair<string, string> PSS;

class config {
public:
    static int port;
    static int backlog;

    static string path_url_error;

    static string path_file_cgi_process;
    static string path_file_error;

    static vector<PSS> list_url2path_static;
    static vector<PSS> list_url2file_cgi;

    static string env_query_string;

    static bool debug;
    static int log_level;

    config() {}
    ~config() {}
};
int config::port;
int config::backlog;
string config::path_url_error;
string config::path_file_cgi_process;
string config::path_file_error;
bool config::debug;
int config::log_level;

vector<PSS> config::list_url2path_static;
vector<PSS> config::list_url2file_cgi;
string config::env_query_string;
#endif // CONFIG_H
