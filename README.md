# Easy Reactor

Easy-Reactor是一个基于Reactor模式的Linux C++网络服务器框架，支持多线程TCP服务器，单线程TCP服务器，单线程UDP服务器

在工作中开发基础服务器的经验总结、以及阅读陈硕《muduo》一书的收获，使得我以沉淀的心态做了这个项目
此项目在我的另一个项目[易用稳定、高性能的服务间远程调用管理、调度、负载系统：Easy-Load-Balancer][1]中得到了全面应用，此项目的实战充分证明了Easy-Reactor服务框架的性能很高、使用也很简单

[1]: https://github.com/LeechanX/Easy-Load-Balancer

### TCP服务器架构：多线程Reactor

![Multi-Thread-Arch](pictures/multi-thread-arch.png)

### Timer Queue设计

- 以最小堆管理Timers（注册Timer、删除Timer），以每个Timer的发生时间在最小堆中排序
- 以timerfd作为通知方式，交给eventLoop监听，将超时事件转为IO事件
- timerfd所设置的时间总是最小堆的堆顶Timer的发生时间

### 使用方法

以一个`pingpong echo server`为例

#### tcp server端：
![Server-Example](pictures/server-example.png)

#### tcp client端：
![Client-Example](pictures/client-example.png)
