#ifndef __TCP_SERVER_H__
#define __TCP_SERVER_H__

#include "event_loop.h"
#include "thread_pool.h"
#include <netinet/in.h>

class tcp_server
{
public:
    tcp_server(const char* ip, uint16_t port);

    ~tcp_server();

    void domain();

    void do_accept();

private:
    int _sockfd;
    int _reservfd;
    event_loop* _loop;
    thread_pool* _thd_pool;
    struct sockaddr_in _connaddr;
    socklen_t _addrlen;
};

#endif
