#include <stdio.h>
#include <time.h>
#include <string>
#include "easy_reactor.h"

void buz1(event_loop* loop, void* usr_data)
{
    FILE* fp = (FILE*)usr_data;
    fprintf(fp, "once display ts %ld\n", time(NULL));
    fflush(fp);
}

void buz2(event_loop* loop, void* usr_data)
{
    FILE* fp = (FILE*)usr_data;
    fprintf(fp, "always display ts %ld\n", time(NULL));
    fflush(fp);
}

int main()
{
    event_loop loop;
    config_reader::setPath("myconf.ini");
    std::string ip = config_reader::ins()->GetString("reactor", "ip", "0.0.0.0");
    short port = config_reader::ins()->GetNumber("reactor", "port", 12315);

    tcp_server server(&loop, ip.c_str(), port);

    FILE* fp = fopen("output.log", "w");
    loop.run_after(buz1, fp, 10);
    loop.run_every(buz2, fp, 1, 500);

    loop.process_evs();

    fclose(fp);
    return 0;
}
