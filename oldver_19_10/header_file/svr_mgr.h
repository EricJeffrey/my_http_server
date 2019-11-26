#if !defined(SVR_MGR)
#define SVR_MGR

#include "req_resp_header.h"

const char msg_body_404[] = "404: Requested Resource Not Found";

typedef pair<string, string> pss;

// exit on error. err: error number, s: info, thread_exit: exit only this thread, sd: sd to close (only when thread_exit is false)
int err_exit(int err, const char *s, bool thread_exit, int sd);

// parse http headers, saved in header
void parse_headers(const char *buf, int len, my_req_header *header);

// handle client connectioin
void *handle_conn(void *x);

// server listen
void start_listen();

/*send a http msg
sd: socket, ver_co_phr: http/1.0 200 OK, vcp_len: len of ver_co_phr
h_body: general header content, msg_body: data, m_body_len: size of data*/
int send_msg(int sd, con_c_str ver_co_phr, int vcp_len, vector<pss> *h_body, con_c_str msg_body, int mb_len);

// send file content into sd
int send_file(string path, int sd);
#endif // SVR_MGR
