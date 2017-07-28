#ifndef __NET_COMMU_H__
#define __NET_COMMU_H__

#include <stdint.h>

class net_commu
{
public:
    virtual void send_data(const char* data, uint32_t datalen, int cmdid) = 0;
};

#endif
