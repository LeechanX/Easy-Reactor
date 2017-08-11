#ifndef __TCP_CLIENT_H__
#define __TCP_CLIENT_H__

#include <unistd.h>
#include "net_commu.h"
#include "io_buffer.h"
#include "event_loop.h"
#include "msg_dispatcher.h"

class tcp_client: public net_commu
{
public:
    tcp_client(event_loop* loop, const char* ip, unsigned short port);

    void add_msg_cb(int cmdid, msg_callback* msg_cb, void* usr_data = NULL) { _dispatcher.add_msg_cb(cmdid, msg_cb, usr_data); }

    void do_connect();

    int send_data(const char* data, int datlen, int cmdid);

    int handle_read();

    int handle_write();

    ~tcp_client() { ::close(_sockfd); }

    void clean_conn();

    bool net_ok;
    io_buffer ibuf, obuf;

private:
    int _sockfd;
    event_loop* _loop;
    struct sockaddr_in _servaddr;
    socklen_t _addrlen;
    msg_dispatcher _dispatcher;
};

#endif
