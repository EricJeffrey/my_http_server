#include "main_app.h"
#include <exception>
#include <execinfo.h>
#include <iostream>

// connection per thread model
int main(int argc, char const *argv[]) {
    try {
        main_app::run();
    } catch (const std::exception &e) {
        std::cerr << "err!\t" << e.what() << '\n';
        return 0;
    }
    return 0;
}
