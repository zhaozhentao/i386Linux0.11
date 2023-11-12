#include <errno.h>
#include <sys/types.h>

#include <linux/sched.h>

extern int tty_read(unsigned minor,char * buf,int count);
extern int tty_write(unsigned minor,char * buf,int count);

typedef int (*crw_ptr)(int rw,unsigned minor,char * buf,int count,off_t * pos);

static int rw_ttyx(int rw,unsigned minor,char * buf,int count,off_t * pos)
{
    // todo tty read
    // 调用 tty 的底层读写操作
    return ((rw==READ)? 0 :
        tty_write(minor,buf,count));
}

#define NRDEVS ((sizeof (crw_table))/(sizeof (crw_ptr)))

static crw_ptr crw_table[]={
    NULL,        /* nodev */
    NULL,        /* /dev/mem etc */
    NULL,        /* /dev/fd */
    NULL,        /* /dev/hd */
    rw_ttyx,     /* /dev/ttyx */
    NULL,      /* /dev/tty */
    NULL,        /* /dev/lp */
    NULL         /* unnamed pipes */
};

int rw_char(int rw,int dev, char * buf, int count, off_t * pos) {
    crw_ptr call_addr;

    if (MAJOR(dev)>=NRDEVS)
        return -ENODEV;
    // 根据主设备号，找到其处理函数地址
    if (!(call_addr=crw_table[MAJOR(dev)]))
        return -ENODEV;
    return call_addr(rw,MINOR(dev),buf,count,pos);
}

