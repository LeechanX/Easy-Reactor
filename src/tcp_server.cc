#include "tcp_server.h"
#include "print_error.h"
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

void accepter_cb(event_loop* loop, int fd, void *args)
{
    tcp_server* server = (tcp_server*)args;
    server->do_accept();
}

tcp_server::tcp_server(const char* ip, uint16_t port)
{
    _sockfd = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP);
    exit_if(_sockfd == -1, "socket()");

    _reservfd = ::open("/tmp/reactor_accepter", O_CREAT | O_RDONLY | O_CLOEXEC, 0666);
    sys_error_if(_reservfd == -1, "open()");

    struct sockaddr_in servaddr;
    servaddr.sin_family = AF_INET;
    int ret = ::inet_aton(ip, &servaddr.sin_addr);
    exit_if(ret == 0, "ip format %s", ip);

    servaddr.sin_port = htons(port);

    ret = ::bind(_sockfd, (const struct sockaddr*)&servaddr, sizeof servaddr);
    exit_if(ret == -1, "bind()");

    ret = ::listen(_sockfd, 500);
    exit_if(ret == -1, "listen()");

    _loop = new event_loop();
    exit_if(_loop == NULL, "new event_loop");

    _addrlen = sizeof (struct sockaddr_in);

    //add accepter event
    _loop->add_ioev(_sockfd, accepter_cb, EPOLLIN | EPOLLET);

    //if mode is multi-thread reactor, create thread pool
    _thd_pool = NULL;
}

tcp_server::~tcp_server()
{
    ::close(_sockfd);
    ::close(_reservfd);
    delete _loop;
}

void tcp_server::do_accept()
{
    int connfd;
    bool conn_full = false;
    while (true)
    {
        connfd = ::accept(_sockfd, (struct sockaddr*)&_connaddr, &_addrlen);
        if (connfd == -1)
        {
            if (errno == EINTR)
            {
                continue;
            }
            else if (errno == EMFILE)
            {
                conn_full = true;
                ::close(_reservfd);
            }
            else if (errno == EAGAIN)
            {
                break;
            }
            else
            {
                exit_if(1, "accept()");
            }
        }
        else if (conn_full)
        {
            ::close(connfd);
            _reservfd = ::open("/tmp/reactor_accepter", O_CREAT | O_RDONLY | O_CLOEXEC, 0666);
            sys_error_if(_reservfd == -1, "open()");
        }
        else
        {
            //connfd
            //multi-thread reactor model: round-robin a event loop and give message to it
            if (_thd_pool)
            {
                thread_queue* cq = _thd_pool->get_next_thread();
                queue_msg msg;
                msg.cmd_type = queue_msg::NEW_CONN;
                msg.connfd = connfd;
                cq->send_msg(msg);
            }
            else//register in self thread
            {

            }
        }
    }
}

void tcp_server::domain()
{
    _loop->process_evs();
}