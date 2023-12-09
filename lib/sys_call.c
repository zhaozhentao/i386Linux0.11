#define __LIBRARY__
#include <unistd.h>

int sys_exit() {
    printk("sys_exit\n");
}

int sys_waitpid() {
    printk("sys_waitpid\n");
}

int sys_creat() {
    printk("sys_creat\n");
}

int sys_chdir() {
    printk("sys_chdir\n");
}

int sys_mknod() {
    printk("sys_mknod\n");
}

int sys_chmod() {
    printk("sys_chmod\n");
}

int sys_chown() {
    printk("sys_chown\n");
}

int sys_break() {
    printk("sys_break\n");
}

int sys_lseek() {
    printk("sys_lseek\n");
}

int sys_getpid() {
    printk("sys_getpid\n");
}

int sys_mount() {
    printk("sys_mount\n");
}

int sys_umount() {
    printk("sys_umount\n");
}

int sys_setuid() {
    printk("sys_setuid\n");
}

int sys_getuid() {
    printk("sys_getuid\n");
}

int sys_stime() {
    printk("sys_stime\n");
}

int sys_ptrace() {
    printk("sys_ptrace\n");
}

int sys_alarm() {
    printk("sys_alarm\n");
}

int sys_fstat() {
    printk("sys_fstat\n");
}

int sys_stty() {
    printk("sys_stty\n");
}

int sys_gtty() {
    printk("sys_gtty\n");
}

int sys_access() {
    printk("sys_access\n");
}

int sys_nice() {
    printk("sys_nice\n");
}
int sys_ftime() {
    printk("sys_ftime\n");
}

int sys_kill() {
    printk("sys_kill\n");
}

int sys_rename() {
    printk("sys_rename\n");
}

int sys_pipe() {
    printk("sys_pipe\n");
}

int sys_times() {
    printk("sys_times\n");
}

int sys_prof() {
    printk("sys_prof\n");
}

int sys_setgid() {
    printk("sys_setgid\n");
}

int sys_getgid() {
    printk("sys_getgid\n");
}

int sys_geteuid() {
    printk("sys_geteuid\n");
}

int sys_getegid() {
    printk("sys_getegid\n");
}

int sys_acct() {
    printk("sys_acct\n");
}

int sys_phys() {
    printk("sys_phys\n");
}
int sys_lock() {
    printk("sys_lock\n");
}

int sys_fcntl() {
    printk("sys_fcntl\n");
}

int sys_mpx() {
    printk("sys_mpx\n");
}

int sys_setpgid() {
    printk("sys_setpgid\n");
}

int sys_uname() {
    printk("sys_uname\n");
}

int sys_umask() {
    printk("sys_umask\n");
}

int sys_chroot() {
    printk("sys_chroot\n");
}

int sys_dup2() {
    printk("sys_dup2\n");
}

int sys_getppid() {
    printk("sys_getppid\n");
}

int sys_getpgrp() {
    printk("sys_getpgrp\n");
}

int sys_setsid() {
    printk("sys_setsid\n");
}

int sys_sgetmask() {
    printk("sys_sgetmask\n");
}

int sys_ssetmask() {
    printk("sys_ssetmask\n");
}

int sys_setreuid() {
    printk("sys_setreuid\n");
}

int sys_setregid() {
    printk("sys_setregid\n");
}

int sys_iam() {
    printk("sys_iam\n");
}

int sys_whoami() {
    printk("sys_whoami\n");
}

