#include <errno.h>
#include <fcntl.h>
#include <assert.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "msg_head.h"
#include "tcp_client.h"
#include "print_error.h"

static void read_cb(event_loop* loop, int fd, void* args)
{
    tcp_client* cli = (tcp_client*)args;
    cli->handle_read();
}

static void write_cb(event_loop* loop, int fd, void* args)
{
    tcp_client* cli = (tcp_client*)args;
    cli->handle_write();
}

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
        cli->net_ok = true;
        loop->add_ioev(fd, read_cb, EPOLLIN | EPOLLET, cli);
        if (cli->obuf.length)
        {
            loop->add_ioev(fd, write_cb, EPOLLOUT, cli);
        }
    }
    else
    {
        //connect build error!
        //reconnection after 2s
        loop->run_after(reconn_cb, cli, 2);
    }
}

tcp_client::tcp_client(event_loop* loop, const char* ip, unsigned short port):
    net_ok(false), 
    ibuf(4194304),
    obuf(4194304),
    _sockfd(-1),
    _loop(loop)
{
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

void tcp_client::do_connect()
{
    if (_sockfd != -1)
        ::close(_sockfd);
    //create socket
    _sockfd = ::socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC | SOCK_NONBLOCK, IPPROTO_TCP);
    exit_if(_sockfd == -1, "socket()");

    int ret = ::connect(_sockfd, (const struct sockaddr*)&_servaddr, _addrlen);
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

int tcp_client::send_data(const char* data, uint32_t datlen, int cmdid)//call by user
{
    bool need = (obuf.length == 0 && net_ok) ? true: false;//if need to add to event loop
    if (datlen + COMMU_HEAD_LENGTH > obuf.capacity - obuf.length)
    {
        error_log("no more space to write socket");
        return -1;
    }
    commu_head head;
    head.cmdid = cmdid;
    head.length = datlen;
    ::memcpy(obuf.data + obuf.length, &head, COMMU_HEAD_LENGTH);

    obuf.length += COMMU_HEAD_LENGTH;
    ::memcpy(obuf.data + obuf.length, data, datlen);
    obuf.length += datlen;

    if (need)
    {
        _loop->add_ioev(_sockfd, write_cb, EPOLLOUT, this);
    }
    return 0;
}

int tcp_client::handle_read()
{
    int rn;
    if (::ioctl(_sockfd, FIONREAD, &rn) == -1)
    {
        error_log("ioctl FIONREAD");
        return -1;
    }
    assert((uint32_t)rn <= ibuf.capacity - ibuf.length);
    while (true)
    {
        int r = ::read(_sockfd, ibuf.data + ibuf.length, rn);
        if (r == -1)
        {
            if (errno == EINTR)
            {
                continue;
            }
            else
            {
                error_log("read()");
                clean_conn();
                return -1;
            }
        }
        else if (r == 0)
        {
            //peer close connection
            error_log("connection closed by peer");
            clean_conn();
            return -1;
        }
        assert(r == rn);
        ibuf.length += r;
        break;
    }
    commu_head head;
    int cmdid, length;
    while (ibuf.length >= COMMU_HEAD_LENGTH)
    {
        ::memcpy(&head, ibuf.data + ibuf.head, COMMU_HEAD_LENGTH);
        cmdid = head.cmdid;
        length = head.length;

        if (length + COMMU_HEAD_LENGTH < ibuf.length)
        {
            //sub-package
            break;
        }

        ibuf.head += COMMU_HEAD_LENGTH;
        ibuf.length -= COMMU_HEAD_LENGTH;

        if (!_dispatcher.exist(cmdid))
        {
            error_log("this message has no corresponding callback, close connection");
            clean_conn();
            return -1;
        }
        _dispatcher.cb(ibuf.data + ibuf.head, length, cmdid, this);

        ibuf.head += length;
        ibuf.length -= length;
    }
    ::memmove(ibuf.data, ibuf.data + ibuf.head, ibuf.length);
    ibuf.head = 0;
    return 0;
}

int tcp_client::handle_write()
{
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
                error_log("write()");
                clean_conn();
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
    else
    {
        obuf.head = 0;
        _loop->del_ioev(_sockfd, EPOLLOUT);
    }
    return 0;
}

void tcp_client::clean_conn()
{
    if (_sockfd != -1)
    {
        _loop->del_ioev(_sockfd);
        ::close(_sockfd);
    }
    //ibuf.clear();
    //obuf.clear();
    net_ok = false;

    //connect
    do_connect();
}
