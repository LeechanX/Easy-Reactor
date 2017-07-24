#include "tcp_conn.h"
#include "msg_head.h"
#include "dispatcher.h"
#include "tcp_server.h"
#include "print_error.h"
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>

static void tcp_rcb(event_loop* loop, int fd, void *args)
{
    tcp_conn* conn = (tcp_conn*)args;
    conn->handle_read();
}

static void tcp_wcb(event_loop* loop, int fd, void *args)
{
    tcp_conn* conn = (tcp_conn*)args;
    conn->handle_write();
}

void tcp_conn::init(int connfd, event_loop* loop)
{
    _connfd = connfd;
    _loop = loop;
    //set NONBLOCK
    int flag = ::fcntl(_connfd, F_GETFL, 0);
    ::fcntl(_connfd, F_SETFL, O_NONBLOCK | flag);
    _loop->add_ioev(_connfd, tcp_rcb, EPOLLIN | EPOLLET, this);

    tcp_server::inc_conn();
}

void tcp_conn::handle_read()
{
    int ret = ibuf.read_data(_connfd);
    if (ret == -1)
    {
        //read data error
        error_log("read data from socket");
        clean_conn();
        return ;
    }
    else if (ret == -2)
    {
        //The peer is closed, return -2
        clean_conn();
        return ;
    }
    req_head header;
    while (ibuf.length() >= REQ_HEAD_LENGTH)
    {
        ::memcpy(&header, ibuf.data(), REQ_HEAD_LENGTH);
        if (header.length > MSG_LENGTH_LIMIT || header.length < 0)
        {
            //data format is messed up
            error_log("data format error in data head, close connection");
            clean_conn();
            break;
        }
        if (ibuf.length() < REQ_HEAD_LENGTH + header.length)
        {
            //this is half-package
            break;
        }
        //find in dispatcher
        msg_callback* cb = dispatcher::ins()->cb(header.cmdid);
        if (!cb)
        {
            //data format is messed up
            error_log("this message has no corresponding callback, close connection");
            clean_conn();
            break;
        }
        ibuf.pop(REQ_HEAD_LENGTH);
        //domain: call user callback
        cb(ibuf.data(), header.length, this);
        ibuf.pop(header.length);
    }
}

void tcp_conn::handle_write()
{
    if (obuf.length())
    {
        int ret = obuf.write_fd(_connfd);
        if (ret == -1)
        {
            error_log("write TCP buffer error, close connection");
            clean_conn();
            return ;
        }
    }
    if (!obuf.length())
    {
        _loop->del_ioev(_connfd, EPOLLOUT);
    }
}

void tcp_conn::send_data(const char* data, uint32_t datlen)
{
    bool need_listen = false;
    if (!obuf.length())
        need_listen = true;
    //write rsp head first
    rsp_head head;
    head.length = datlen;
    obuf.send_data((const char*)&head, RSP_HEAD_LENGTH);
    //write content
    obuf.send_data(data, datlen);
    if (need_listen)
    {
        _loop->add_ioev(_connfd, tcp_wcb, EPOLLOUT, this);
    }
}

void tcp_conn::clean_conn()
{
    _loop->del_ioev(_connfd);
    _loop = NULL;
    ::close(_connfd);
    _connfd = -1;
    ibuf.clear();
    obuf.clear();

    tcp_server::dec_conn();
}