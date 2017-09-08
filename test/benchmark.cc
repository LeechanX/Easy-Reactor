#include <stdio.h>
#include <time.h>
#include <string>
#include <vector>
#include <stdlib.h>
#include <sys/time.h>
#include "echoMsg.pb.h"
#include "easy_reactor.h"

struct Config
{
    Config(): hostip(NULL), hostPort(0), concurrency(0), total(0) {}
    char* hostip;
    short hostPort;
    int concurrency;
    long total;
};

unsigned long getCurrentMills()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (unsigned long)tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

Config config;
unsigned long startTs, endTs;

void parseOption(int argc, char** argv)
{
    for (int i = 0;i < argc; ++i)
    {
        if (!strcmp(argv[i], "-h"))
        {
            config.hostip = argv[i + 1];
        }
        else if (!strcmp(argv[i], "-p"))
        {
            config.hostPort = atoi(argv[i + 1]);
        }
        else if (!strcmp(argv[i], "-c"))
        {
            config.concurrency = atoi(argv[i + 1]);
        }
        else if (!strcmp(argv[i], "-n"))
        {
            config.total = atol(argv[i + 1]);
        }
    }
    if (!config.hostip || !config.hostPort || !config.concurrency || !config.total)
    {
        printf("./dss-benchmark -h ip -p port -c concurrency -n total\n");
        exit(1);
    }
}

void echoBack(const char* data, uint32_t len, int msgId, net_commu* commu, void* usr_data)
{
    long* count = (long*)usr_data;
    echo::EchoString req, rsp;
    if (rsp.ParseFromArray(data, len) && rsp.content() == "I miss you i miss you i miss you i miss you i miss you i miss you i miss you i miss you i miss you!")
        *count = *count + 1;

    if (*count >= config.total)
    {
        endTs = getCurrentMills();
        printf("communicate %ld times\n", *count);
        printf("time use %ldms\n", endTs - startTs);
        printf("qps %.2f\n", (*count * 1000.0) / (endTs - startTs));
        exit(1);
    }
    req.set_id(rsp.id() + 1);
    req.set_content(rsp.content());
    std::string reqStr;
    req.SerializeToString(&reqStr);
    commu->send_data(reqStr.c_str(), reqStr.size(), 1);//回复消息
}

void onConnectionCb(tcp_client* client, void *args)
{
    unsigned long* startTsPtr = (unsigned long*)args;
    if (!*startTsPtr)
        *startTsPtr = getCurrentMills();
    //连接建立后，主动发送消息
    echo::EchoString req;
    req.set_id(100);
    req.set_content("I miss you i miss you i miss you i miss you i miss you i miss you i miss you i miss you i miss you!");
    std::string reqStr;
    req.SerializeToString(&reqStr);
    client->send_data(reqStr.c_str(), reqStr.size(), 1);//主动发送消息
}

int main(int argc, char** argv)
{
    parseOption(argc, argv);
    long done = 0;
    event_loop loop;
    std::vector<tcp_client*> clients;
    for (int i = 0;i < config.concurrency; ++i)
    {
        tcp_client* cli = new tcp_client(&loop, config.hostip, config.hostPort);//创建TCP客户端
        if (!cli) exit(1);
        cli->add_msg_cb(1, echoBack, &done);//设置：当收到消息id=GetRouteByAgentRspId的消息时的回调函数
        cli->onConnection(onConnectionCb, &startTs);//当连接建立后，执行函数onConnectionCb
        clients.push_back(cli);
    }

    loop.process_evs();

    for (int i = 0;i < config.concurrency; ++i)
    {
        tcp_client* cli = clients[i];
        delete cli;
    }
    return 0;
}
