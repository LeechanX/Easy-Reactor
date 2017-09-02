#include <time.h>
#include <stdio.h>
#include <pthread.h>
#include <iostream>
#include "echoMsg.pb.h"
#include "easy_reactor.h"

using namespace std;
using namespace echo;

struct testQPS
{
    testQPS(): lstTs(0), succ(0) {}
    long lstTs;
    int succ;
};

void buz(const char* data, uint32_t len, int cmdid, net_commu* commu, void* usr_data)
{
    testQPS* qps = (testQPS*)usr_data;//获取用户参数
    EchoString req, rsp;
    if (rsp.ParseFromArray(data, len) && rsp.content() == "I miss you i miss you i miss you i miss you i miss you i miss you i miss you i miss you i miss you!")
    {
        qps->succ++;
    }
    long curTs = time(NULL);
    if (curTs - qps->lstTs >= 1)
    {
        printf(" ** qps:%d **\n", qps->succ);
        qps->lstTs = curTs;
        qps->succ = 0;
    }

    req.set_id(rsp.id() + 1);
    req.set_content(rsp.content());

    string reqStr;
    req.SerializeToString(&reqStr);
    commu->send_data(reqStr.c_str(), reqStr.size(), cmdid);//回复消息
}

void whenConnectDone(tcp_client* client, void* args)
{
    EchoString req;
    req.set_id(100);
    req.set_content("I miss you i miss you i miss you i miss you i miss you i miss you i miss you i miss you i miss you!");
    string reqStr;
    req.SerializeToString(&reqStr);
    client->send_data(reqStr.c_str(), reqStr.size(), 1);//主动发送消息
}

void* domain(void* args)
{
    event_loop loop;
    tcp_client client(&loop, "127.0.0.1", 12315);//创建TCP客户端

    testQPS qps;
    client.add_msg_cb(1, buz, (void*)&qps);//设置：当收到消息id=1的消息时的回调函数
    //安装连接建立后调用的回调函数
    client.onConnection(whenConnectDone);

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
