# 又是HTTP服务器呢

**一服务一线程模型**，实现静态文件映射+动态CGI程序执行。

使用配置文件配置如下项:

- 静态文件路径
- CGI程序url
- CGI程序管理器路径

## bug tracker
- 如下日志中，返回404之后的 `readLinefillbuffer return 0`错误
  ```
  11628 log       server started, listening on port 8686
  11628 log       connection from: 192.168.199.1 established
  11628 log       connection from: 192.168.199.1 established
  11630 log       handling new conn from: 192.168.199.1
  11630 log       reading request from socket: 5
  11630 log       start serve static file with url: static/abc
  failed with errno: 2, operator() stat file failed No such file or directory.
  failed with errno: 2, serveStatic ./static/abc doesnot exist No such file or directory.
  failed with errno: 2, handleNewConn serve static file failed No such file or directory.
  11630 log       start serve error file, status_code: 500
  11629 log       handling new conn from: 192.168.199.1
  11629 log       reading request from socket: 4
  failed with errno: 0, readLinefillbuffer return 0.
  failed with errno: 0, readRequestHeaderreader.readline return 0.
  failed with errno: 0, handleNewConnreadRequestHeader failed.
  11629 log       start serve error file, status_code: 500
  11628 log       connection from: 192.168.199.1 established
  11628 log       connection from: 192.168.199.1 established
  11632 log       handling new conn from: 192.168.199.1
  11632 log       reading request from socket: 4
  11632 log       start serve static file with url: static/halo
  failed with errno: 0, serveStatic./static/halo not a regular file.
  11632 log       start serve error file, status_code: 404
  11631 log       handling new conn from: 192.168.199.1
  11631 log       reading request from socket: 6
  ```
- 
