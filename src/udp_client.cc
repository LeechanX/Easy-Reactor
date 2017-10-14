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
#include "udp_client.h"
#include "print_error.h"

void read_cb(event_loop* loop, int fd, void *args)
{
    udp_client* server = (udp_client*)args;
    server->handle_read();
}

udp_client::udp_client(event_loop* loop, const char* ip, uint16_t port)
{
    //create socket
    _sockfd = ::socket(AF_INET, SOCK_DGRAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_UDP);
    exit_if(_sockfd == -1, "socket()");

    struct sockaddr_in servaddr;
    ::bzero(&servaddr, sizeof (servaddr));
    servaddr.sin_family = AF_INET;
    int ret = ::inet_aton(ip, &servaddr.sin_addr);
    exit_if(ret == 0, "ip format %s", ip);
    servaddr.sin_port = htons(port);

    ret = ::connect(_sockfd, (const struct sockaddr*)&servaddr, sizeof servaddr);
    exit_if(ret == -1, "connect()");

    _loop = loop;

    //add accepter event
    _loop->add_ioev(_sockfd, read_cb, EPOLLIN, this);
}

udp_client::~udp_client()
{
    _loop->del_ioev(_sockfd);
    ::close(_sockfd);
}

void udp_client::handle_read()
{
    while (true)
    {
        int pkg_len = ::recvfrom(_sockfd, _rbuf, sizeof _rbuf, 0, NULL, NULL);
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
        if (head.length > MSG_LENGTH_LIMIT || head.length < 0 || head.length != pkg_len - COMMU_HEAD_LENGTH)
        {
            //data format is messed up
            error_log("data format error in data head");
            continue;
        }
        _dispatcher.cb(_rbuf + COMMU_HEAD_LENGTH, head.length, head.cmdid, this);
    }
}

int udp_client::send_data(const char* data, int datlen, int cmdid)
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

    int ret = ::sendto(_sockfd, _wbuf, datlen + COMMU_HEAD_LENGTH, 0, NULL, 0);
    if (ret == -1)
    {
        error_log("sendto()");
    }
    return ret;
}
