# MyWebServer

一个linux系统下的C++轻量级Web服务器，用以学习和实践网络编程。经测试支持上万并发连接。

- C/C++
- B/S模型
- reactor事件处理模式
- ET模式非阻塞IO的epoll
- 支持长连接和处理粘包的连接类
- 半同步/半反应堆线程池
- 时间轮实现的定时器处理非活动连接
- 支持HTTP协议的get方法

TODO:实现对POST方法的支持

TODO:更完整的应答报文头部

### 编译环境

- Ubuntu 20.04
- gcc 9.3.0
- cmake 3.16
- c++17

### 编译过程

>cmake .
>
>make

### 使用方法

直接运行MyWebServer时使用默认端口号2132。使用-p 端口号 选项可设置服务器运行的端口号。

### 压力测试

使用webbench进行上万并发的压力测试通过。

> webbench -c 10000 -t 10 http://localhost:2132/

[![63XjNF.png](https://s3.ax1x.com/2021/03/09/63XjNF.png)](https://imgtu.com/i/63XjNF)
