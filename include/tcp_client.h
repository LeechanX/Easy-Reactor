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
    tcp_client(event_loop* loop, const char* ip, unsigned short port, const char* name = NULL);

    typedef void onconn_func(tcp_client* client, void* args);
    typedef void oncls_func(tcp_client* client, void* args);

    //set up function after connection ok
    void onConnection(onconn_func* func, void *args = NULL)
    {
        _onconnection = func;
        _onconn_args = args;
    }

    //set up function after connection closed
    void onClose(oncls_func* func, void *args = NULL)
    {
        _onclose = func;
        _onclose_args = args;
    }

    void call_onconnect()
    {
        if (_onconnection)
            _onconnection(this, _onconn_args);
    }

    void call_onclose()
    {
        if (_onclose)
            _onclose(this, _onclose_args);
    }

    void add_msg_cb(int cmdid, msg_callback* msg_cb, void* usr_data = NULL) { _dispatcher.add_msg_cb(cmdid, msg_cb, usr_data); }

    void do_connect();

    virtual int send_data(const char* data, int datlen, int cmdid);

    virtual int get_fd() { return _sockfd; }

    int handle_read();

    int handle_write();

    ~tcp_client() { ::close(_sockfd); }

    void clean_conn();

    event_loop* loop() { return _loop; }

    bool net_ok;
    io_buffer ibuf, obuf;
    struct sockaddr_in servaddr;
private:
    int _sockfd;
    event_loop* _loop;
    socklen_t _addrlen;
    msg_dispatcher _dispatcher;
    //when connection success, call _onconnection(_onconn_args)
    onconn_func* _onconnection;
    void* _onconn_args;
    //when connection close, call _onclose(_onclose_args)
    oncls_func* _onclose;
    void* _onclose_args;
    const char* _name;
};

#endif
