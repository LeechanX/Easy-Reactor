#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <strings.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "udp_server.h"
#include "print_error.h"

void read_cb(event_loop* loop, int fd, void *args)
{
    udp_server* server = (udp_server*)args;
    server->handle_read();
}

udp_server::udp_server(event_loop* loop, const char* ip, uint16_t port)
{
    //ignore SIGHUP and SIGPIPE
    if (::signal(SIGHUP, SIG_IGN) == SIG_ERR)
    {
        error_log("signal ignore SIGHUP");
    }

    //create socket
    _sockfd = ::socket(AF_INET, SOCK_DGRAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_UDP);
    exit_if(_sockfd == -1, "socket()");

    struct sockaddr_in servaddr;
    ::bzero(&servaddr, sizeof (servaddr));
    servaddr.sin_family = AF_INET;
    int ret = ::inet_aton(ip, &servaddr.sin_addr);
    exit_if(ret == 0, "ip format %s", ip);
    servaddr.sin_port = htons(port);

    ret = ::bind(_sockfd, (const struct sockaddr*)&servaddr, sizeof servaddr);
    exit_if(ret == -1, "bind()");

    _loop = loop;

    ::bzero(&_srcaddr, sizeof (_srcaddr));
    _addrlen = sizeof (struct sockaddr_in);

    info_log("server on %s:%u is running...", ip, port);

    //add accepter event
    _loop->add_ioev(_sockfd, read_cb, EPOLLIN, this);
}

udp_server::~udp_server()
{
    _loop->del_ioev(_sockfd);
    ::close(_sockfd);
}

void udp_server::handle_read()
{
    while (true)
    {
        int pkg_len = ::recvfrom(_sockfd, _rbuf, sizeof _rbuf, 0, (struct sockaddr *)&_srcaddr, &_addrlen);
        if (pkg_len == -1)
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
                error_log("recfrom()");
                break;
            }
        }
        //handle package _rbuf[0:pkg_len)
        commu_head head;
        ::memcpy(&head, _rbuf, COMMU_HEAD_LENGTH);
        if (head.length > MSG_LENGTH_LIMIT || head.length < 0 || head.length + COMMU_HEAD_LENGTH != pkg_len)
        {
            //data format is messed up
            error_log("data format error in data head, head[%d,%d], pkg_len %d", head.length, head.cmdid, pkg_len);
            continue;
        }
        _dispatcher.cb(_rbuf + COMMU_HEAD_LENGTH, head.length, head.cmdid, this);
    }
}

int udp_server::send_data(const char* data, int datlen, int cmdid)
{
    if (datlen > MSG_LENGTH_LIMIT)
    {
        error_log("udp response length too large");
        return -1;
    }
    commu_head head;
    head.length = datlen;
    head.cmdid = cmdid;

    ::memcpy(_wbuf, &head, COMMU_HEAD_LENGTH);
    ::memcpy(_wbuf + COMMU_HEAD_LENGTH, data, datlen);

    int ret = ::sendto(_sockfd, _wbuf, datlen + COMMU_HEAD_LENGTH, 0, (struct sockaddr *)&_srcaddr, _addrlen);
    if (ret == -1)
    {
        error_log("sendto()");
    }
    return ret;
}
