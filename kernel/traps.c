#include <linux/head.h>
#include <asm/system.h>

void divide_error(void);
void debug(void);
void nmi(void);
void int3(void);
void overflow(void);
void bounds(void);
void invalid_op(void);
void device_not_available(void);
void double_fault(void);
void coprocessor_segment_overrun(void);
void invalid_TSS(void);
void segment_not_present(void);
void stack_segment(void);
void general_protection(void);
void page_fault(void);
void coprocessor_error(void);
void reserved(void);
void parallel_interrupt(void);
void irq13(void);

void do_double_fault(long esp, long error_code) {

}

void do_general_protection(long esp, long error_code) {

}

void do_divide_error(long esp, long error_code) {

}

void do_nmi(long esp, long error_code) {

}

void do_overflow(long esp, long error_code) {

}

void do_bounds(long esp, long error_code) {

}

void do_invalid_op(long esp, long error_code) {

}

void do_coprocessor_segment_overrun(long esp, long error_code) {

}

void do_invalid_TSS(long esp,long error_code) {

}

void do_segment_not_present(void) {

}

void do_stack_segment() {

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
) {

}

void do_reserved(long esp, long error_code) {

}

void trap_init(void) {
    int i;

    set_trap_gate(0, &divide_error);           // 1 / 0 就可以触发除零异常
    set_trap_gate(1, &debug);                  // __asm__("int $1" : : ); 触发异常
    set_trap_gate(2, &nmi);                    // 硬件中断，触发方式未知
    set_system_gate(3, &int3);                 // __asm__("int $3" : : ); 触发异常 int3-5 can be called from all
    set_system_gate(4, &overflow);             // 触发方式未知 最大 int 数值 + 1 不能触发 overflow 异常
    set_system_gate(5, &bounds);               // 边界溢出异常, 触发方式未知
    set_trap_gate(6, &invalid_op);
    set_trap_gate(7, &device_not_available);   // 数学处理器不存在异常
    set_trap_gate(8, &double_fault);
    set_trap_gate(9, &coprocessor_segment_overrun);
    set_trap_gate(10, &invalid_TSS);
    set_trap_gate(11, &segment_not_present);
    set_trap_gate(12, &stack_segment);
    set_trap_gate(13, &general_protection);
    set_trap_gate(14, &page_fault);
    set_trap_gate(15, &reserved);
}

