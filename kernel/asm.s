.global divide_error

divide_error:
  pushl $do_divide_error                         # 将 do_divide_error 地址入栈,如果直接将这个地址放到寄存器里面，寄存器原来的数据就丢失了,所以先入栈,再交换到寄存器
no_error_code:
  xchgl  %eax,(%esp)                             # 将栈里面的，比如上面的(do_divide_error) 放到 %eax 寄存器，%eax 寄存器的数据替换栈里面的,目的是为了获取处理函数的同时，保护现场
  pushl  %ebx                                    # 下面几个指令都是保护现场 将当前的寄存器值保存至栈
  pushl  %ecx
  pushl  %edx
  pushl  %edi
  pushl  %esi
  pushl  %ebp
  push   %ds
  push   %es
  push   %fs
  pushl  $0                                      # "error code"
  lea    44(%esp), %edx                          # 将前面保存处理函数地址的地址取出，然后入栈
  pushl  %edx                                    # 入栈
  movl   $0x10, %edx                             # 初始化段寄存器 ds es fs ，加载内核数据段选择符
  mov    %dx, %ds
  mov    %dx, %es
  mov    %dx, %fs
  call   *%eax                                   # 根据保存在 eax 寄存器的地址，调用处理函数，比如 do_divide_error
  addl   $8, %esp                                # 将函数入口地址 和 错误号出栈
  pop    %fs                                     # 将前面入栈的寄存器值出栈，恢复现场
  pop    %es
  pop    %ds
  popl   %ebp
  popl   %esi
  popl   %edi
  popl   %edx
  popl   %ecx
  popl   %ebx
  popl   %eax
  iret

