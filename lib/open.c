#define __LIBRARY__
#include <unistd.h>
#include <stdarg.h>

int open(const char * filename, int flag, ...)
{
    register int res;
    va_list arg;
    // 通过 int 0x80 发起软中断，进入内核空间
    va_start(arg,flag);
    __asm__("int $0x80"
        :"=a" (res)
        :"0" (__NR_open),"b" (filename),"c" (flag),
        "d" (va_arg(arg,int)));
    if (res>=0)
        return res;
    errno = -res;
    return -1;
}

