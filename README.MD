# 简单HTTP服务器

**一服务一线程模型**，实现静态文件映射+动态CGI程序执行。

## 目标
- 配置目录映射
- 配置自定义错误页面
- 使用Python脚本编写RestCGI程序
  - 映射指定Python模块的url

## 设计记录

1. 处理连接流程
   ```txt
   开始 -- 读取请求首部 -- 解析请求方法、路径与参数 --GET-- 确定路径类型 --文件-- 响应文件 -- 结束
                                                |                   |
                                                |                   --CGI--- 启动CGI程序 -- 读取输出字符串 -- 封装响应内容 -- 结束
                                                |                   |
                                                |                   --其它--  响应404 -- 结束
                                                |
                                                --POST-- 确定路径类型 --文件-- 响应405不运行 -- 结束
                                                                    |
                                                                     --CGI--- 读取请求体 -- 启动CGI程序 -- 读取输出字符串 -- 封装相应内容 -- 结束
                                                                    |
                                                                     --其它-- 响应404 -- 结束
   ```
2. 文件、CGI程序映射
   1. 需求
      1. 静态文件: url路径(**../abc/**) => 绝对路径(目录)  
      2. cgi程序: url路径(**.../abc**) => 绝对路径(py文件)  
         1. 通过环境变量传参数
         2. 通过标准输出获取response
         3. exit-code不为0代表错误
         4. 不区分错误类型
      
   2. CGI执行流程 - 已知待执行程序为 A.py, 参数为 paras
      ```
      1. 检查文件是否存在
      2. 阻塞SIGCHLD信号
      3. 创建管道pipe
      4. fork
         1. 将paras放入环境变量env
         2. dup标准输出stdou到pipe
         3. 执行A.py
      5. 等待pipe数据(子进程结束后pipe输出什么？)
      6. 取消阻塞信号
      7. 查询进程退出状态
      8. 封装数据，响应客户端
      ```
3. 命令行参数+配置文件
   1. 需求
      1. 命令行参数(优先): `-p port(int) -a address(a.b.c.d) -d debug(true/false) -l loglevel(info/verbose) -o outputfile(string)`
      2. 配置文件: 
         ```conf
            port=int
            address=a.b.c.d
            timeout_sec_conn=float
            debug=true/false
            log_level=info/verbose
            path_logger=path
            list_url2path_static=[
               A B
               ...
            ]
            list_url2file_cgi=[
               A B
               ...
            ]
            map_code2file_error={
               404 B
               500 C
               ...
            }
         ```
   2. 实现方式
      1. 命令行参数: `GNU getopt` + `c++ regex`
      2. 配置文件格式: 单行文本`A=B`，数组使用`A=[B,C]`，键值对使用`{A=B,C=D}`; 按照ASCII字符串撸代码解析

## Bug记录
- wget结束后程序终止，套接字上的`read`操作返回0
  > 对于`SOCK_STREAM`，非阻塞IO下返回0意味着套接字对端关闭
- `addr.sin_addr.s_addr = INADDR_ANY`造成bind出错
  > 应当为`addr.sin_addr.s_addr = htonl(INADDR_ANY)`
- 使用`--static`编译程序并调用`thread.detach()`方法时，运行抛出错误
  > `g++ ... -Wl,--whole-archive -lpthread -Wl,--no-whole-archive` 将整个libpthread.a都链接到程序中即可

## 待办Todo
近期需要处理的任务

- 配置信息的重新设计 - cgi程序目录 + 静态文件映射（多个） **√**
- cgi管理程序 - 封装输出、响应客户端请求  **√**
- 错误文件映射    **√**
- 实现持久性连接，timeout，暂不考虑请求首部的`keep-alive`与`timeout`    **√**
  > 只需在发送一个请求后不关闭socket即可，客户端会pipeline请求
- 简单计时: 超时timeout值  **√**
  > 每个连接都在线程内，不断地从连接读取输入并判断是否超时即可  
  > 需要设置连接为非阻塞的
- 删除不必要日志输出 **√**
- 程序生命周期管理 - 处理Ctrl+C(SIGINT)信号，结束子进程、线程并退出 **√**
- 日志管理器，确保日志输出不冲突 **√**
  - 使用单独一个线程管理日志输出
- 从文件读取配置信息，命令行参数
- 解析请求头的首部
  - 使用客户端指定的timeout值
  - 传输编码encoding
- 解析GET参数
  - 确定url中的查询参数query string是否需要解析 - 目前不要
- 解析POST参数
- 测试
- 文件encode类型 gzip deflate 如何使用