#ifndef __PRINT_ERROR_H__
#define __PRINT_ERROR_H__

#include <stdio.h>
#include <errno.h>
#include <stdarg.h>
#include <string.h>

#define error_if(condition, fmt, args...) \
    do { \
        if (condition) { \
            fprintf(stderr, "ERROR:" fmt "\n", ##args); \
        } \
    } while (0)

#define sys_error_if(condition, fmt, args...) \
    do { \
        if (condition) { \
            fprintf(stderr, "ERROR: %s when " fmt "\n", strerror(errno), ##args); \
        } \
    } while (0)

inline void exit_if(int condition, const char *fmt, ...)
{
    va_list arglist;

    if (condition)
    {
        va_start(arglist, fmt);
        fprintf(stderr, "ERROR: ");
        vfprintf(stderr, fmt, arglist);
        if (errno)
        {
            fprintf(stderr, ": %s", strerror(errno));
        }
        fprintf(stderr, "\n");
        va_end(arglist);
        exit(EXIT_FAILURE);   
    }
}

#endif
