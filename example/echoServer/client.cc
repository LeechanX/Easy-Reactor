#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <time.h>
#include "echoMsg.pb.h"
#include "easy_reactor.h"

using namespace std;
using namespace echo;

void buz(const char* data, uint32_t len, int cmdid, net_commu* commu)
{
    EchoString req, rsp;

    rsp.ParseFromArray(data, len);

    req.set_id(rsp.id() + 1);
    req.set_content(rsp.content());

    string reqStr;
    req.SerializeToString(&reqStr);
    commu->send_data(reqStr.c_str(), reqStr.size(), cmdid);
}

void* domain(void* args)
{
    #if 0
    int succ = 0;
    int sockfd = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    long lstTs = time(NULL);

    struct sockaddr_in servaddr;
    servaddr.sin_family = AF_INET;
    inet_aton("127.0.0.1", &servaddr.sin_addr);
    servaddr.sin_port = htons(12315);

    ::connect(sockfd, (const struct sockaddr*)&servaddr, sizeof servaddr);
    EchoString rsp;
    while (true)
    {
        send_msg(sockfd, "i am leechanx");
        if (decode_msg(sockfd, rsp) == 0)
        {
            ++succ;
        }
        long currentTs = time(NULL);
        if (currentTs > lstTs)
        {
            printf("success communicate %d\n", succ);
            succ = 0;
            lstTs = currentTs;
        }
    }
    ::close(sockfd);
    #endif

    event_loop loop;

    tcp_client client(&loop, "127.0.0.1", 12315);
    client.add_msg_cb(1, buz);

    EchoString req;
    req.set_id(100);
    req.set_content("My Name Is LeechanX, I Miss Helen Liang Very Much");
    string reqStr;
    req.SerializeToString(&reqStr);
    client.send_data(reqStr.c_str(), reqStr.size(), 1);

    loop.process_evs();
    return NULL;
}

int main(int argc, char** argv)
{
    int threadNum = atoi(argv[1]);
    pthread_t *tids;
    tids = new pthread_t[threadNum];
    for (int i = 0;i < threadNum; ++i)
        pthread_create(&tids[i], NULL, domain, NULL);
    for (int i = 0;i < threadNum; ++i)
        pthread_join(tids[i], NULL);
    return 0;
}