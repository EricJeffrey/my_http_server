#if !defined(CONFIG_H)
#define CONFIG_H
#include <string>
#include <unordered_map>
#include <vector>
using std::pair;
using std::string;
using std::unordered_map;
using std::vector;

typedef pair<string, string> PAIR_SS;
typedef pair<int, string> PAIR_IS;

class config {
public:
    static string address;
    static int port;
    static int backlog;

    static vector<PAIR_SS> list_url2path_static;
    static vector<PAIR_SS> list_url2file_cgi;
    static unordered_map<int, string> map_code2file_error;

    static string key_env_query_string;

    // timeout of connection
    static double timeout_sec_conn;

    static bool debug;
    static int log_level;

    static string path_logger;

    config() {}
    ~config() {}

    // todo load configuration from file
    static void load() {}
};

string config::address;
int config::port;
int config::backlog;

bool config::debug;
int config::log_level;

vector<PAIR_SS> config::list_url2path_static;
vector<PAIR_SS> config::list_url2file_cgi;
unordered_map<int, string> config::map_code2file_error;

double config::timeout_sec_conn;

string config::key_env_query_string;

string config::path_logger;

#endif // CONFIG_H
