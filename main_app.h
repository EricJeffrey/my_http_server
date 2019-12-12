#if !defined(MAIN_APP_H)
#define MAIN_APP_H

#include <set>
#include <string>
#include <unistd.h>
using std::set;
using std::string;

enum state {
    initialize,
    start,
    stop
};

class main_app {
private:
    // arguments to parse
    static int argc;
    static char **argv;
    static const string usage;
    static set<pid_t> set_child_pids;

    static int createConfigFile();
    static int parseArgs();
    static int loadConfig();
    static void init();
    static void start();
    static void stop();

public:
    static state app_state;

    main_app() {}
    ~main_app() {}

    static void run(int argc, char *argv[]) {
        main_app::argc = argc;
        main_app::argv = argv;
        init();
        start();
    }
    static void addChildProcess(pid_t pid) {
        set_child_pids.insert(pid);
    }
    static void removeChild(pid_t pid) {
        set_child_pids.erase(pid);
    }
};
#endif // MAIN_APP_H
