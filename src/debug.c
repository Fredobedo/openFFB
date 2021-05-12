#include "debug.h"

#include <stdarg.h>

int globalLevel = 0;

int initDebug(int level)
{
    globalLevel = level;
    return 1;
}

void debug(int level, const char *format, ...)
{
    if (globalLevel < level)
        return;

    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
}

void debugBuffer(int level, unsigned char *buffer, int length)
{
    if (globalLevel < level)
        return;

    for (int i = 0; i < length; i++)
        printf("0x%02hhX ", buffer[i]);
    printf("\n");
}


