#include <string>
#include "echoMsg.pb.h"
#include "easy_reactor.h"

using namespace std;
using namespace echo;

void buz(const char* data, uint32_t len, int cmdid, net_commu* commu, void* usr_data)
{
    EchoString req, rsp;
    req.ParseFromArray(data, len);//解包，data[0:len)保证是一个完整包
    rsp.set_id(req.id());
    rsp.set_content(req.content());
    string rspStr;
    rsp.SerializeToString(&rspStr);
    commu->send_data(rspStr.c_str(), rspStr.size(), cmdid);//回复消息
}

int main()
{
    event_loop loop;
    udp_server server(&loop, "127.0.0.1", 12315, "myconf.ini");//创建TCP服务器
    server.add_msg_cb(1, buz);//设置：当收到消息id = 1的消息调用的回调函数  我们约定EchoString消息的ID是1
    loop.process_evs();
    return 0;
}
