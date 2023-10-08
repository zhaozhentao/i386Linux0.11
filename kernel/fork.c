#include <errno.h>

#include <linux/sched.h>

long last_pid=0;

// 为新进程取得不重复的进程号，返回任务数组中的任务号(数组项)
int find_empty_process(void) {
    int i;

    repeat:
        // 如果 +1 后超出正整数范围，重置为 1
        if ((++last_pid)<0) last_pid=1;
        // 检查 last_pid 是否已经存在在任务数组中，如果是就重新获取一个 last_pid，last_pid 是全局变量不用返回
        for(i=0 ; i<NR_TASKS ; i++)
            if (task[i] && task[i]->pid == last_pid) goto repeat;
    // 在任务数组中找一个空项，返回项号
    for(i=1 ; i<NR_TASKS ; i++)
        if (!task[i])
            return i;
    return -EAGAIN;
}

