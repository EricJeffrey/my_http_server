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
    // timeout of connection
    static double timeout_sec_conn;
    static string path_logger;
    static vector<PAIR_SS> list_url2file_cgi;
    static vector<PAIR_SS> list_url2path_static;
    static unordered_map<int, string> map_code2file_error;
    // specfied in argumemtns
    static bool debug;
    // specfied in argumemtns
    static int log_level;

    // predefined values
    static const string key_port;
    static const string key_address;
    static const string key_timeout_sec_conn;
    static const string key_path_logger;
    static const string key_list_url2path_static;
    static const string key_list_url2file_cgi;
    static const string key_map_code2file_error;
    static const string path_default_logger_cerr;
    static const string path_config;
    static const string key_env_query_string;

    config() {}
    ~config() {}
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

string config::path_logger;

const string config::key_port = "port";
const string config::key_address = "address";
const string config::key_timeout_sec_conn = "connection_timeout";
const string config::key_path_logger = "log_file";
const string config::key_list_url2path_static = "static_directory_map";
const string config::key_list_url2file_cgi = "cgi_file_map";
const string config::key_map_code2file_error = "error_file_map";
const string config::key_env_query_string = "query";
const string config::path_default_logger_cerr = "cerr";
const string config::path_config = "halox.conf";

#endif // CONFIG_H
