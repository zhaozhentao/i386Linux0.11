#include <linux/head.h>
#include <asm/system.h>

void divide_error(void);
void debug(void);

void do_divide_error(long esp, long error_code) {

}

void do_int3(
    long * esp,
    long error_code,
    long fs,
    long es,
    long ds,
    long ebp,
    long esi,
    long edi,
    long edx,
    long ecx,
    long ebx,
    long eax
){

}

void trap_init(void) {
    int i;

    set_trap_gate(0, &divide_error);           // 1 / 0 就可以触发除零异常
    set_trap_gate(1, &debug);                  // __asm__("int $1" : : ); 触发异常

}

