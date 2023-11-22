extern int sys_setup();
extern int sys_fork();
extern int sys_write();
extern int sys_open();
extern int sys_close();
extern int sys_link();
extern int sys_unlink();
extern int sys_utime();
extern int sys_mkdir();
extern int sys_rmdir();
extern int sys_dup();
extern int sys_ustat();

fn_ptr sys_call_table[] = {
    sys_setup,
    0,
    sys_fork,
    0,
    sys_write,
    sys_open,
    sys_close,
    0,
    0,
    sys_link,
    sys_unlink,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    sys_utime,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    sys_mkdir,
    sys_rmdir,
    sys_dup,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    sys_ustat,
};

