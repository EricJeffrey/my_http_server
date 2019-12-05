#include <arpa/inet.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <pthread.h>
#include <string>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

using std::pair;
using std::string;
using std::vector;

typedef const char *con_c_str;

#define SVR_PORT 2500
#define BACKLOG 20
#define PAGE_DIR_PATH "./pages"

#if !defined(ERR_EXIT)
#define ERR_EXIT
int err_exit(int err, const char *s, bool thread_exit, int sd);
#endif // ERR_EXIT
