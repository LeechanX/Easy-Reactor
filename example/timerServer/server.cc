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
    event_loop loop;
    tcp_server server(&loop, "127.0.0.1", 12315, "myconf.ini");

    FILE* fp = fopen("output.log", "w");
    loop.run_after(buz1, fp, 10);
    loop.run_every(buz2, fp, 1, 500);

    loop.process_evs();

    fclose(fp);
    return 0;
}
