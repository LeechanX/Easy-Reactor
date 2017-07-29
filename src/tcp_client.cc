#ifndef __TCP_CLIENT_H__
#define __TCP_CLIENT_H__

#include <errno.h>
#include <fcntl.h>
#include <assert.h>
#include <stdint.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "print_error.h"
#include "event_loop.h"

class tcp_client;

static void reconn_cb(event_loop* loop, void* usr_data)
{
    tcp_client* cli = (tcp_client*)usr_data;
    cli->do_connect();
}

static void connection_cb(event_loop* loop, int fd, void* args)
{
    tcp_client* cli = (tcp_client*)args;
    loop->del_ioev(fd);
    int result;
    socklen_t result_len = sizeof(result);
    getsockopt(fd, SOL_SOCKET, SO_ERROR, &result, &result_len);
    if (result == 0)
    {
        //connect build success!
        error_log("connection OK!");//debug
        net_ok = true;
        loop->add_ioev(fd, cli->rcb, EPOLLIN, cli);
    }
    else
    {
        error_log("connection error");//debug
        //connect build error!
        //reconnection after 2s
        loop->run_after(reconn_cb, cli, 2);
    }
}

static void write_cb(event_loop* loop, int fd, void* args)
{
    tcp_client* cli = (tcp_client*)args;
    
}

class tcp_client
{
public:
    tcp_client(event_loop* loop, const char* ip, unsigned short port, io_callback* rcb, 
        io_callback* wcb):
    net_ok(false), 
    ibuf(262144),
    obuf(4194304),
    _sockfd(-1),
    _loop(loop)
    {
        assert(rcb);
        assert(wcb);
        this->rcb = rcb;
        this->wcb = wcb;
        //ignore SIGHUP and SIGPIPE
        if (::signal(SIGHUP, SIG_IGN) == SIG_ERR)
        {
            error_log("signal ignore SIGHUP");
        }
        
        //construct server address
        _servaddr.sin_family = AF_INET;
        int ret = ::inet_aton(ip, &_servaddr.sin_addr);
        exit_if(ret == 0, "ip format %s", ip);
        _servaddr.sin_port = htons(port);
        _addrlen = sizeof _servaddr;

        //connect
        do_connect();
    }

    void do_connect()
    {
        if (_sockfd != -1)
            ::close(_sockfd);
        //create socket
        _sockfd = ::socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC | SOCK_NONBLOCK, IPPROTO_TCP);
        exit_if(_sockfd == -1, "socket()");

        ret = ::connect(_sockfd, (const struct sockaddr*)&_servaddr, _addrlen);
        if (ret == 0)
        {
            net_ok = true;
            error_log("connection finish");//debug
        }
        else
        {
            if (errno == EINPROGRESS)
            {
                //add connection event
                error_log("connection is doing");//debug
                _loop->add_ioev(_sockfd, connection_cb, EPOLLOUT, this);
            }
            else
            {
                exit_log("connect()");
            }
        }
    }

    int send_data(const char* data, uint32_t len)//call by user
    {
        bool need = obuf.length? true: false;//if need to add to event loop
        if (len > obuf.capacity - obuf.length)
        {
            error_log("no more space to write socket");
            return -1;
        }
        ::memcpy(obuf.data + obuf.length, data, len);
        obuf.length += len;
        if (need)
        {
            _loop->add_ioev(_sockfd, write_cb, EPOLLOUT, this);
        }
        return 0;
    }

    int handle_read()
    {
        bool need = false;
        
    }

    int handle_write()
    {
        bool need = false;
        while (obuf.length)
        {
            int w = ::write(_sockfd, obuf.data + obuf.head, obuf.length);
            if (w == -1)
            {
                if (errno == EINTR)
                {
                    continue;
                }
                else if (errno == EAGAIN)
                {
                    break;
                }
                else
                {
                    clean_conn();
                    error_log("write()");
                    return -1;
                }
            }
            else
            {
                obuf.head += w;
                obuf.length -= w;
            }
        }
        if (obuf.length)
        {
            ::memmove(obuf.data, obuf.data + obuf.head, obuf.length);
            obuf.head = 0;
        }
    }

    ~tcp_client()
    {
        ::close(_sockfd);
    }

    void clean_conn()
    {
        if (_sockfd != -1)
        {
            _loop->del_ioev(_sockfd);
            ::close(_sockfd);
        }
        ibuf.clear();
        obuf.clear();
        net_ok = false;

        //connect
        do_connect();
    }

    bool net_ok;
    io_callback* rcb;
    io_callback* wcb;
    io_buffer ibuf, obuf;

private:
    int _sockfd;
    event_loop* _loop;
    struct sockaddr_in _servaddr;
    socklen_t _addrlen;
};

#endif
