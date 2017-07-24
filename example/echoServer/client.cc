#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "echoMsg.pb.h"
#include "easy_reactor.h"

using namespace std;
using namespace echo;

int send_msg(int sockfd, const char* content)
{
    static int id = 0;
    req_head head;
    head.cmdid = 1;

    EchoString req;
    req.set_id(id++);
    req.set_content(content);
    string str;
    req.SerializeToString(&str);

    head.length = str.size();
    char wbuf[1024] = {};
    ::memcpy(wbuf, &head, REQ_HEAD_LENGTH);
    ::memcpy(wbuf + REQ_HEAD_LENGTH, str.c_str(), str.size());
    int wl = REQ_HEAD_LENGTH + str.size();

    int ret = ::write(sockfd, wbuf, wl);
    if (ret != wl)
        perror("write");
    return 0;
}

int decode_msg(int sockfd, EchoString& rsp)
{
    char rbuf[1024];
    rsp_head head;
    int rn = ::read(sockfd, &head, RSP_HEAD_LENGTH);
    if (rn > 0 && rn != RSP_HEAD_LENGTH)
    {
        printf("read head get length != RSP_HEAD_LENGTH\n");
        return -1;
    }
    else if (rn == 0)
    {
        printf("server closed connection\n");
        return -1;
    }
    else if (rn == -1)
    {
        perror("read");
        return -1;
    }
    int length = head.length;
    rn = ::read(sockfd, rbuf, length);
    if (rn > 0 && rn != length)
    {
        perror("read content get length != RSP_HEAD_LENGTH\n");
        return -1;
    }
    else if (rn == 0)
    {
        printf("server closed connection\n");
        return -1;
    }
    else if (rn == -1)
    {
        perror("read");
        return -1;
    }
    rsp.ParseFromArray(rbuf, length);
    return 0;
}

int main()
{
    int sockfd = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    struct sockaddr_in servaddr;
    servaddr.sin_family = AF_INET;
    inet_aton("127.0.0.1", &servaddr.sin_addr);
    servaddr.sin_port = htons(12315);

    ::connect(sockfd, (const struct sockaddr*)&servaddr, sizeof servaddr);
    EchoString rsp;
    int i = 0;
    while (i++ < 10)
    {
        send_msg(sockfd, "i am leechanx");
        if (decode_msg(sockfd, rsp) == 0)
        {
            printf("content = %s\n", rsp.content().c_str());
        }
    }
    ::close(sockfd);
}