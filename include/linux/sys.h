extern int sys_setup();
extern int sys_fork();
extern int sys_write();
extern int sys_open();

fn_ptr sys_call_table[] = {sys_setup, sys_fork, sys_fork, sys_open, sys_write, sys_open};

