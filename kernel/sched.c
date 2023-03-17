#include <linux/sched.h>

long user_stack [ PAGE_SIZE>>2 ] ;

// 这里 0x10 是内核段选择符
struct {
  long * a;
  short b;
} stack_start = { & user_stack [PAGE_SIZE>>2] , 0x10 };

// todo: this method
void math_state_restore() {

}

