#include <errno.h>

#include <linux/sched.h>
#include <asm/segment.h>
#include <sys/times.h>
#include <sys/utsname.h>

int sys_ulimit() {
    return -ENOSYS;
}

int sys_time(long * tloc)
{
    int i;
    
    i = CURRENT_TIME;
    if (tloc) {
        verify_area(tloc,4);
        put_fs_long(i,(unsigned long *)tloc);
    }
    return i;
}


int sys_brk(unsigned long end_data_seg)
{
    if (end_data_seg >= current->end_code &&
        end_data_seg < current->start_stack - 16384)
        current->brk = end_data_seg;
    return current->brk;
}

int sys_uname(struct utsname * name)
{
    static struct utsname thisname = {
        "linux .0","nodename","release ","version ","machine "
    };
    int i;
    
    if (!name) return -ERROR;
        verify_area(name,sizeof *name);
    for(i=0;i<sizeof *name;i++)
        put_fs_byte(((char *) &thisname)[i],i+(char *) name);
    return 0;
}

