.global divide_error, debug, nmi, int3, overflow, bounds, invalid_op, double_fault
.global coprocessor_segment_overrun, invalid_TSS

divide_error:
  pushl $do_divide_error              # 将 do_divide_error 地址入栈,如果直接将这个地址放到寄存器里面，寄存器原来的数据就丢失了,所以先入栈,再交换到寄存器
no_error_code:
  xchgl %eax, (%esp)                  # 将栈里面的，比如上面的(do_divide_error) 放到 %eax 寄存器，%eax 寄存器的数据替换栈里面的,目的是为了获取处理函数的同时，保护现场
  pushl %ebx                          # 下面几个指令都是保护现场
  pushl %ecx
  pushl %edx
  pushl %edi
  pushl %esi
  pushl %ebp
  push  %ds
  push  %es
  push  %fs
  pushl $0                            # 错误号
  lea   44(%esp), %edx                # 将前面保存处理函数地址的地址取出，然后入栈
  pushl %edx                          # 入栈
  movl  $0x10, %edx                   # 初始化段寄存器 ds es fs ，加载内核数据段选择符
  mov   %dx, %ds
  mov   %dx, %es
  mov   %dx, %fs
  call  *%eax                         # 根据保存在 eax 寄存器的地址，调用处理函数，比如 do_divide_error
  addl  $8, %esp                      # 相当于出栈 c 函数的两个入参
  pop   %fs                           # 下面都是出栈
  pop   %es
  pop   %ds
  popl  %ebp
  popl  %esi
  popl  %edi
  popl  %edx
  popl  %ecx
  popl  %ebx
  popl  %eax
  iret

debug:
  pushl $do_int3                      # _do_debug
  jmp   no_error_code

nmi:
  pushl $do_nmi
  jmp   no_error_code

int3:
  pushl $do_int3
  jmp   no_error_code

overflow:
  pushl $do_overflow
  jmp   no_error_code

bounds:
  pushl $do_bounds
  jmp   no_error_code

invalid_op:
  pushl $do_invalid_op
  jmp   no_error_code

coprocessor_segment_overrun:
  pushl $do_coprocessor_segment_overrun
  jmp   no_error_code


double_fault:
  pushl $do_double_fault
error_code:
  xchgl %eax, 4(%esp)             # error code <-> %eax
  xchgl %ebx, (%esp)              # &function <-> %ebx
  pushl %ecx
  pushl %edx
  pushl %edi
  pushl %esi
  pushl %ebp
  push  %ds
  push  %es
  push  %fs
  pushl %eax                      # error code
  lea   44(%esp), %eax            # offset
  pushl %eax
  movl  $0x10, %eax
  mov   %ax, %ds
  mov   %ax, %es
  mov   %ax, %fs
  call  *%ebx
  addl  $8, %esp
  pop   %fs
  pop   %es
  pop   %ds
  popl  %ebp
  popl  %esi
  popl  %edi
  popl  %edx
  popl  %ecx
  popl  %ebx
  popl  %eax
  iret

invalid_TSS:
  pushl $do_invalid_TSS
  jmp   error_code

