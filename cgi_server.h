#if !defined(CGI_SERVER_H)
#define CGI_SERVER_H

#include "config.h"
#include "logger.h"
#include "response_header.h"
#include "utils.h"
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <signal.h>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>
#include <wait.h>

using std::pair;
using std::string;
using std::vector;

// 执行 [python3 path_prog]
int startCgiProg(int fds_pipe[], const string &path_prog, const vector<string> &list_paras) {
    int ret = 0;
    close(fds_pipe[0]);
    string query_string;
    for (size_t i = 0; i < list_paras.size(); i++) {
        if (i != 0) query_string += '&';
        query_string += list_paras[i];
    }
    setenv(config::key_env_query_string.c_str(), query_string.c_str(), 1);
    ret = dup2(fds_pipe[1], STDOUT_FILENO);
    if (ret == -1) {
        logger::fail({"in ", __func__, ": call to dup2 failed"}, true);
        return -1;
    }
    char *path_cstr = strdup(path_prog.c_str());
    char *path_exe = strdup("python3");
    char *argv[] = {path_exe, path_cstr, NULL};
    execvp(path_exe, argv);
    logger::fail({"in ", __func__, ": call to execvp failed"}, true);
    return -1;
}

// 执行 [python3 path_prog]，并将标准输出的内容发送到 [sd],
// -1 for error, 0 success, -2 404
int serveCgi(const string &path_prog, const vector<string> &list_paras, int sd) {
    int ret = 0;
    ret = utils::isRegFile(path_prog.c_str());
    // check file accessibility
    if (ret == -1 || ret == 0) {
        logger::info({__func__, " call to utils.isRegFile failed"});
        return -2;
    }
    if (access(path_prog.c_str(), F_OK | R_OK) == -1) {
        logger::info({__func__, " call to access failed, file not found or cannot read"});
        return -2;
    }
    int fds_pipe[2];
    // create pipe
    ret = pipe(fds_pipe);
    if (ret == -1) {
        logger::fail({"in ", __func__, ": call to pipe failed"}, true);
        return -1;
    }
    sigset_t mask, prev;
    sigemptyset(&mask);
    sigaddset(&mask, SIGCHLD);
    sigprocmask(SIG_BLOCK, &mask, &prev);
    // start child
    pid_t pid_child = fork();
    if (pid_child == -1) {
        logger::fail({"in ", __func__, ": call to fork failed"}, true);
        return -1;
    } else if (pid_child == 0) {
        sigprocmask(SIG_SETMASK, &prev, NULL);
        // child
        ret = startCgiProg(fds_pipe, path_prog, list_paras);
        if (ret == -1) {
            logger::fail({"in ", __func__, ": call to startCgiProg failed"});
            return -1;
        }
    } else if (pid_child > 0) {
        // father, add child to list
        main_app::addChildProcess(pid_child);
        // close read
        close(fds_pipe[1]);
        string data;
        ret = utils::readAll(fds_pipe[0], data);
        if (ret == -1) {
            logger::fail({"in ", __func__, ": call to utils.readAll failed"});
            return -1;
        } else if (ret == -2) {
            logger::fail({"in ", __func__, ": call to utils.readAll failed, would block!"});
            return -1;
        }
        logger::debug({"cgi server, read child ", to_string(ret), " bytes"});
        int status_child;
        sigprocmask(SIG_SETMASK, &prev, nullptr);
        // start waitpid
        ret = waitpid(pid_child, &status_child, 0);
        if (ret == -1) {
            logger::fail({"in ", __func__, ": call to waitpid failed"}, true);
            return -1;
        }
        // remove child
        main_app::removeChild(pid_child);
        const int exit_code_child = WEXITSTATUS(status_child);
        // terminated with 0
        if (exit_code_child == 0 || WIFSIGNALED(status_child)) {
            // send response
            response_header header;
            response_header::strHeader(data, header);
            string resp = header.toString() + "\r\n" + data;
            ret = utils::writeStr2Fd(resp, sd);
            if (ret == -1) {
                logger::fail({"in ", __func__, ": call of writeStr2Fd failed"});
                return -1;
            }
            return 0;
        } else {
            logger::fail({"in ", __func__, ": cgi prog: ", path_prog, " exit with non-zero value: ", to_string(exit_code_child)});
            return -1;
        }
    }
    return 0;
}
#endif // CGI_SERVER_H
