.global rs1_interrupt, rs2_interrupt

.align 2
rs1_interrupt:
  jmp rs_int

.align 2
rs2_interrupt:

rs_int:
  pushl %edx                         # 保护现场
  pushl %ecx
  pushl %ebx
  pushl %eax
  push  %es
  push  %ds                          /* as this is an interrupt, we cannot */
  pushl $0x10                        /* know that bs is ok. Load it */
  pop   %ds                          # 将上面入栈的 0x10 pop 到 ds 数据段寄存器中，表示指向内核数据段
  pushl $0x10
  pop   %es                          # 将上面入栈的 0x10 pop 到 es 附加段寄存器中，表示指向内核数据段

