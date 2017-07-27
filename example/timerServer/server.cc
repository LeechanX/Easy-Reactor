#include <stdio.h>
#include <time.h>
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
    tcp_server server("127.0.0.1", 12315, "myconf.ini");
    FILE* fp = fopen("output.log", "w");
    server.loop()->run_after(buz1, fp, 10);
    server.loop()->run_every(buz2, fp, 1, 500);
    server.domain();
    fclose(fp);
    return 0;
}
