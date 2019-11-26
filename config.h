#if !defined(CONFIG_H)
#define CONFIG_H
#include <string>
using std::string;
/**
 * url,path - start without '/'
*/
class config {
public:
    static int port;
    static int backlog;

    static string path_url_static;
    static string path_url_cgi;
    static string path_url_error;

    static string path_dir_static_root;
    static string path_file_cgi_manager;

    static bool debug;
    static int log_level;

    config() {}
    ~config() {}
};
int config::port;
int config::backlog;
string config::path_url_static;
string config::path_url_cgi;
string config::path_url_error;
string config::path_dir_static_root;
string config::path_file_cgi_manager;
bool config::debug;
int config::log_level;
#endif // CONFIG_H
