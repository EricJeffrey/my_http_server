
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>

#if !defined(ERR_HANDLER_H)
#define ERR_HANDLER_H

FILE *fp_err_out = stderr;

#define errExitSimp(A)                                                                           \
    do {                                                                                         \
        fprintf(fp_err_out, "ERROR! %s.\nerrno: %d, strerror: %s\n", A, errno, strerror(errno)); \
        kill(0, SIGKILL);                                                                        \
        exit(EXIT_FAILURE);                                                                      \
    } while (0);

#define errExitFormat(A, ...)                                                            \
    do {                                                                                 \
        fprintf(fp_err_out, "ERROR! ---\n");                                             \
        fprintf(fp_err_out, A, __VA_ARGS__);                                             \
        fprintf(fp_err_out, ".\nerrno: %d, strerror: %s ---\n", errno, strerror(errno)); \
        kill(0, SIGKILL);                                                                \
        exit(EXIT_FAILURE);                                                              \
    } while (0)

#endif // ERR_HANDLER_H
