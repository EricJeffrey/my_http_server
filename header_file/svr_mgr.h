#include "req_resp_header.h"

/*
exit on error. 
err: error number  s: info  
thread_exit: exit only this thread  sd: sd to close (only when thread_exit is false)
*/
int err_exit(int err, const char *s, bool thread_exit, int sd);

// parse http headers, saved in header
void parse_headers(const char *buf, int len, my_req_header *header);

// handle client connectioin
void *handle_conn(void *x);

// server listen
void start_listen();