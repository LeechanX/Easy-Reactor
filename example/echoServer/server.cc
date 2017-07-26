#include <string>
#include "echoMsg.pb.h"
#include "easy_reactor.h"

using namespace std;
using namespace echo;

void buz(const char* data, uint32_t len, net_commu* commu)
{
    EchoString req, rsp;
    req.ParseFromArray(data, len);
    rsp.set_id(req.id() * (-1));
    rsp.set_content(req.content());
    string rspStr;
    rsp.SerializeToString(&rspStr);
    commu->send_data(rspStr.c_str(), rspStr.size());
}

int main()
{
    tcp_server server("127.0.0.1", 12315, "myconf.ini");
    dispatcher::ins()->add_msg_cb(1, buz);
    server.domain();
    return 0;
}
