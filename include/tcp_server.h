#ifndef __TCP_SERVER_H__
#define __TCP_SERVER_H__

#include "event_loop.h"
#include "thread_pool.h"
#include "tcp_conn.h"
#include <netinet/in.h>

class tcp_server
{
public:
    tcp_server(const char* ip, uint16_t port, const char* conf_path);

    ~tcp_server();

    void domain();

    void do_accept();

    static void inc_conn();
    static void get_conn_num(int& cnt);
    static void dec_conn();

private:
    int _sockfd;
    int _reservfd;
    event_loop* _loop;
    thread_pool* _thd_pool;
    struct sockaddr_in _connaddr;
    socklen_t _addrlen;

    static int _conns_size;
    static int _max_conns;
    static int _curr_conns;
    static pthread_mutex_t _mutex;
public:
    static tcp_conn** conns;
};

#endif
