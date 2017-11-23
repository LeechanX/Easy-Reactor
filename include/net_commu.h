#ifndef __NET_COMMU_H__
#define __NET_COMMU_H__

#include <stdint.h>

class net_commu
{
public:
    net_commu(): parameter(NULL) {}

    virtual int send_data(const char* data, int datalen, int cmdid) = 0;
    virtual int get_fd() = 0;

    void* parameter;//每个TCP客户端连接类可以使用此参数设置自己的连接内变量
};

#endif
