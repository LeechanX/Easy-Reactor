#include <string>
#include "echoMsg.pb.h"
#include "easy_reactor.h"

using namespace std;
using namespace echo;

void buz(const char* data, uint32_t len, int cmdid, net_commu* commu, void* usr_data)
{
    EchoString req, rsp;
    req.ParseFromArray(data, len);
    rsp.set_id(req.id() * (-1));
    rsp.set_content(req.content());
    string rspStr;
    rsp.SerializeToString(&rspStr);
    commu->send_data(rspStr.c_str(), rspStr.size(), cmdid);
}

int main()
{
    event_loop loop;
    tcp_server server(&loop, "127.0.0.1", 12315, "myconf.ini");
    server.add_msg_cb(1, buz);

    loop.process_evs();

    return 0;
}
