#include <errno.h>
#include <fcntl.h>
#include <assert.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>
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
        info_log("connect %s:%d successfully", ::inet_ntoa(cli->servaddr.sin_addr), ntohs(cli->servaddr.sin_port));

        //call on connection callback(if has)
        cli->call_onconnect();

        loop->add_ioev(fd, read_cb, EPOLLIN, cli);
        if (cli->obuf.length)
        {
            loop->add_ioev(fd, write_cb, EPOLLOUT, cli);
        }
    }
    else
    {
        //connect build error!
        //reconnection after 2s
        info_log("connect %s:%d error, retry after 2s", ::inet_ntoa(cli->servaddr.sin_addr), ntohs(cli->servaddr.sin_port));
        loop->run_after(reconn_cb, cli, 2);
    }
}

tcp_client::tcp_client(event_loop* loop, const char* ip, unsigned short port, const char* name):
    net_ok(false), 
    ibuf(4194304),
    obuf(4194304),
    _sockfd(-1),
    _loop(loop),
    _onconnection(NULL),
    _onconn_args(NULL),
    _onclose(NULL),
    _onclose_args(NULL),
    _name(name)
{
    //construct server address
    ::bzero(&servaddr, sizeof (servaddr));
    servaddr.sin_family = AF_INET;
    int ret = ::inet_aton(ip, &servaddr.sin_addr);
    exit_if(ret == 0, "ip format %s", ip);
    servaddr.sin_port = htons(port);
    _addrlen = sizeof servaddr;

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

    int ret = ::connect(_sockfd, (const struct sockaddr*)&servaddr, _addrlen);
    if (ret == 0)
    {
        net_ok = true;
        //call on connection callback(if has)
        call_onconnect();

        info_log("connect %s:%d successfully", ::inet_ntoa(servaddr.sin_addr), ntohs(servaddr.sin_port));
    }
    else
    {
        if (errno == EINPROGRESS)
        {
            //add connection event
            _loop->add_ioev(_sockfd, connection_cb, EPOLLOUT, this);
        }
        else
        {
            exit_log("connect()");
        }
    }
}

int tcp_client::send_data(const char* data, int datlen, int cmdid)//call by user
{
    if (!net_ok)
        return -1;
    bool need = (obuf.length == 0) ? true: false;//if need to add to event loop
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
    //一次性读出来所有数据
    assert(net_ok);
    int rn;
    if (::ioctl(_sockfd, FIONREAD, &rn) == -1)
    {
        error_log("ioctl FIONREAD");
        return -1;
    }
    assert(rn <= ibuf.capacity - ibuf.length);
    int ret;
    do
    {
        ret = ::read(_sockfd, ibuf.data + ibuf.length, rn);
    } while (ret == -1 && errno == EINTR);

    if (ret == 0)
    {
        //peer close connection
        if (_name)
            info_log("%s client: connection closed by peer", _name);
        else
            info_log("client: connection closed by peer");
        clean_conn();
        return -1;
    }
    else if (ret == -1)
    {
        assert(errno != EAGAIN);
        error_log("read()");
        clean_conn();
        return -1;
    }
    assert(ret == rn);
    ibuf.length += ret;

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

        ibuf.pop(COMMU_HEAD_LENGTH);

        if (!_dispatcher.exist(cmdid))
        {
            error_log("this message has no corresponding callback, close connection");
            clean_conn();
            return -1;
        }
        _dispatcher.cb(ibuf.data + ibuf.head, length, cmdid, this);

        ibuf.pop(length);
    }
    ibuf.adjust();
    return 0;
}

int tcp_client::handle_write()
{
    assert(obuf.head == 0 && obuf.length);
    int ret;
    while (obuf.length)
    {
        do
        {
            ret = ::write(_sockfd, obuf.data, obuf.length);
        } while (ret == -1 && errno == EINTR);
        if (ret > 0)
        {
            obuf.pop(ret);
            obuf.adjust();
        }
        else if (ret == -1 && errno != EAGAIN)
        {
            error_log("write()");
            clean_conn();
            return -1;
        }
        else
        {
            //此时不可继续写
            break;
        }
    }

    if (!obuf.length)
    {
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
    net_ok = false;

    //call callback when on connection close
    call_onclose();

    //connect
    do_connect();
}
