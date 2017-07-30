# Easy Reactor

### TCP服务器架构：多线程Reactor

![Multi-Thread-Arch](pictures/multi-thread-arch.png)

### Timer Queue设计

- 以最小堆管理Timers（注册Timer、删除Timer），以每个Timer的发生时间在最小堆中排序
- 以timerfd作为通知方式，交给eventLoop监听，将超时事件转为IO事件
- timerfd所设置的时间总是最小堆的堆顶Timer的发生时间

### 使用方法

以一个统计QPS的`pingpong echo server`为例

#### tcp server端：
![Server-Example](pictures/server-example.png)

#### tcp client端：
![Client-Example](pictures/client-example.png)
