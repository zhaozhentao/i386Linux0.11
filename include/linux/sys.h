extern int sys_setup();
extern int sys_fork();
extern int sys_open();

fn_ptr sys_call_table[] = {sys_setup, 0, sys_fork, 0, 0, sys_open};

