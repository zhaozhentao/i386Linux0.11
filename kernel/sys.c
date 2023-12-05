#include <errno.h>

#include <linux/sched.h>
#include <asm/segment.h>
#include <sys/times.h>

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

