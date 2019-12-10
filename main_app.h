#if !defined(MAIN_APP_H)
#define MAIN_APP_H

#include <set>
#include <unistd.h>
using std::set;

enum state {
    initialize,
    start,
    stop
};

class main_app {
private:
    static set<pid_t> set_child_pids;
    static void init();
    static void start();
    static void stop();

public:
    static state app_state;

    main_app() {}
    ~main_app() {}

    static void run() {
        init();
        start();
    }
    static void addChild(pid_t pid) {
        set_child_pids.insert(pid);
    }
    static void removeChild(pid_t pid) {
        set_child_pids.erase(pid);
    }
};

#endif // MAIN_APP_H
