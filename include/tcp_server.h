#ifndef __TCP_SERVER_H__
#define __TCP_SERVER_H__

#include "event_loop.h"
#include "thread_pool.h"
#include "net_commu.h"
#include "tcp_conn.h"
#include "msg_dispatcher.h"
#include <netinet/in.h>

class tcp_server
{
public:
    tcp_server(event_loop* loop, const char* ip, uint16_t port);

    ~tcp_server();//tcp_server类使用时往往具有程序的完全生命周期，其实并不需要析构函数

    void keep_alive() { _keepalive = true; }

    void do_accept();

    void add_msg_cb(int cmdid, msg_callback* msg_cb, void* usr_data = NULL) { dispatcher.add_msg_cb(cmdid, msg_cb, usr_data); }

    static void inc_conn();
    static void get_conn_num(int& cnt);
    static void dec_conn();

    event_loop* loop() { return _loop; }

    thread_pool* threadPool() { return _thd_pool; }

private:
    int _sockfd;
    int _reservfd;
    event_loop* _loop;
    thread_pool* _thd_pool;
    struct sockaddr_in _connaddr;
    socklen_t _addrlen;
    bool _keepalive;

    static int _conns_size;
    static int _max_conns;
    static int _curr_conns;
    static pthread_mutex_t _mutex;
public:
    static msg_dispatcher dispatcher;
    static tcp_conn** conns;

    typedef void (*conn_callback)(net_commu* com);

    static conn_callback connBuildCb;//用户设置连接建立后的回调函数
    static conn_callback connCloseCb;//用户设置连接释放后的回调函数

    static void onConnBuild(conn_callback cb) { connBuildCb = cb; }
    static void onConnClose(conn_callback cb) { connCloseCb = cb; }
};

#endif
