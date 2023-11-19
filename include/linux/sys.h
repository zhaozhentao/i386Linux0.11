extern int sys_setup();
extern int sys_fork();
extern int sys_write();
extern int sys_open();
extern int sys_close();
extern int sys_execve();
extern int sys_dup();

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
    0,
    0,
    sys_execve,
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
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    sys_dup,
};

