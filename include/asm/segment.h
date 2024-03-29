static inline unsigned char get_fs_byte(const char * addr) {
    unsigned register char _v;

    __asm__ ("movb %%fs:%1,%0":"=r" (_v):"m" (*addr));
    return _v;
}

static inline unsigned long get_fs_long(const unsigned long *addr)
{
    unsigned long _v;

    __asm__ ("movl %%fs:%1,%0":"=r" (_v):"m" (*addr)); \
    return _v;
}

