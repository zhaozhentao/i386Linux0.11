#ifndef __GNUC__
#error I want gcc!
#endif

#define extern
#define inline
#define static
#define __LIBRARY__


inline char * strcpy(char * dest,const char *src) {
    __asm__("cld\n"
        "1:\tlodsb\n\t"
        "stosb\n\t"
        "testb %%al,%%al\n\t"
        "jne 1b"
        ::"S" (src),"D" (dest));
    return dest;
}

static inline char * strncpy(char * dest,const char *src,int count)
{
    __asm__("cld\n"
        "1:\tdecl %2\n\t"
        "js 2f\n\t"
        "lodsb\n\t"
        "stosb\n\t"
        "testb %%al,%%al\n\t"
        "jne 1b\n\t"
        "rep\n\t"
        "stosb\n"
        "2:"
        ::"S" (src),"D" (dest),"c" (count));
    return dest;
}

inline int strcmp(const char * cs,const char * ct) {
    register int __res ;
    __asm__("cld\n"
        "1:\tlodsb\n\t"
        "scasb\n\t"
        "jne 2f\n\t"
        "testb %%al,%%al\n\t"
        "jne 1b\n\t"
        "xorl %%eax,%%eax\n\t"
        "jmp 3f\n"
        "2:\tmovl $1,%%eax\n\t"
        "jl 3f\n\t"
        "negl %%eax\n"
        "3:"
        :"=a" (__res):"D" (cs),"S" (ct));
    return __res;
}

// 如果字符串 1 = 字符串 2 返回 0
static inline int strncmp(const char * cs,const char * ct,int count)
{
    register int __res ;
    __asm__("cld\n"
        "1:\tdecl %3\n\t"
        "js 2f\n\t"
        "lodsb\n\t"
        "scasb\n\t"
        "jne 3f\n\t"
        "testb %%al,%%al\n\t"
        "jne 1b\n"
        "2:\txorl %%eax,%%eax\n\t"
        "jmp 4f\n"
        "3:\tmovl $1,%%eax\n\t"
        "jl 4f\n\t"
        "negl %%eax\n"
        "4:"
        :"=a" (__res):"D" (cs),"S" (ct),"c" (count));
    return __res;
}

static inline char * strchr(const char * s,char c)
{
    register char * __res ;
    __asm__("cld\n\t"
        "movb %%al,%%ah\n"
        "1:\tlodsb\n\t"
        "cmpb %%ah,%%al\n\t"
        "je 2f\n\t"
        "testb %%al,%%al\n\t"
        "jne 1b\n\t"
        "movl $1,%1\n"
        "2:\tmovl %1,%0\n\t"
        "decl %0"
        :"=a" (__res):"S" (s),"0" (c));
    return __res;
}

inline int strlen(const char * s)
{
        register int __res ;
        __asm__("cld\n\t"
                "repne\n\t"
                "scasb\n\t"
                "notl %0\n\t"
                "decl %0"
                :"=c" (__res):"D" (s),"a" (0),"0" (0xffffffff));
        return __res;
}

static inline void * memset(void * s,char c,int count) {
    __asm__("cld\n\t"
        "rep\n\t"
        "stosb"
        ::"a" (c),"D" (s),"c" (count)
        );
    return s;
}

