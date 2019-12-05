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
    static int port;
    static int backlog;

    static string path_url_error;
    static string path_file_error;
    static vector<PAIR_SS> list_url2path_static;
    static vector<PAIR_SS> list_url2file_cgi;
    static unordered_map<int, string> map_code2file_error;

    static string env_query_string_key;

    // timeout of connection
    static double timeout_sec_conn;
    // timeout of read/recv/send/sendmsg on socket
    static double timeout_sec_sock;

    static bool debug;
    static int log_level;

    config() {}
    ~config() {}
};
int config::port;
int config::backlog;
double config::timeout_sec_conn;
double config::timeout_sec_sock;

string config::env_query_string_key;
#endif // CONFIG_H
