# My HTTP Server 
A http server written by c/c++, ongoing...
## Done
- map files in ./pages
- handle connection in subthread
## Todo
- figure out how http reuse socket
- fix `double free or corruption (!prev) Aborted (core dumped)` problem
- file data too large, split and send -- part success
- map html files in specific DIR
- request body too large, multiple recv
- read configuration from file(no idea about configurations for now)
  - file_type, port, backlog, urlpath