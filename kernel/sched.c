#include <linux/sched.h>
#include <asm/system.h>

long startup_time=0;

long user_stack [ PAGE_SIZE>>2 ] ;

// 这里 0x10 是内核段选择符
struct {
  long * a;
  short b;
} stack_start = { & user_stack [PAGE_SIZE>>2] , 0x10 };

union task_union {
    struct task_struct task;
    char stack[PAGE_SIZE];
};

static union task_union init_task = {INIT_TASK,};

// todo: this method
void math_state_restore() {

}

void sched_init(void) {
    int i;
    struct desc_struct * p;

    set_tss_desc(gdt+FIRST_TSS_ENTRY,&(init_task.task.tss));
}

