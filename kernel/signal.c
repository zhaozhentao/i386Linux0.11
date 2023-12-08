#include <linux/sched.h>

#include <signal.h>

int sys_signal(int signum, long handler, long restorer) {
    struct sigaction tmp;
    
    if (signum<1 || signum>32 || signum==SIGKILL)
        return -1;
    tmp.sa_handler = (void (*)(int)) handler;
    tmp.sa_mask = 0;
    tmp.sa_flags = SA_ONESHOT | SA_NOMASK;
    tmp.sa_restorer = (void (*)(void)) restorer;
    handler = (long) current->sigaction[signum-1].sa_handler;
    current->sigaction[signum-1] = tmp;
    return handler;
}

