#include <linux/head.h>
#include <asm/system.h>

void divide_error(void);
void debug(void);
void nmi(void);
void int3(void);
void overflow(void);
void bounds(void);

void do_divide_error(long esp, long error_code) {

}

void do_nmi(long esp, long error_code) {

}

void do_overflow(long esp, long error_code) {

}

void do_bounds(long esp, long error_code)
{

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
    set_trap_gate(2, &nmi);                    // 硬件中断，触发方式未知
    set_system_gate(3, &int3);                 // __asm__("int $3" : : ); 触发异常 int3-5 can be called from all
    set_system_gate(4, &overflow);             // 触发方式未知 最大 int 数值 + 1 不能触发 overflow 异常
    set_system_gate(5, &bounds);               // 边界溢出异常, 触发方式未知
}

