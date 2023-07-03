#ifndef _SCHED_H
#define _SCHED_H

#include <linux/head.h>
#include <linux/fs.h>
#include <linux/mm.h>

#define HZ 100

extern long volatile jiffies;
extern long startup_time;

#define CURRENT_TIME (startup_time+jiffies/HZ)

#endif

