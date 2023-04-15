.global divide_error

divide_error:
  pushl $do_divide_error
no_error_code:
  xchgl  %eax,(%esp)
  pushl  %ebx
  pushl  %ecx
  pushl  %edx
  pushl  %edi
  pushl  %esi
  pushl  %ebp
  push   %ds
  push   %es
  push   %fs
  pushl  $0         # "error code"
  lea    44(%esp), %edx
  pushl  %edx
  movl   $0x10, %edx
  mov    %dx, %ds
  mov    %dx, %es
  mov    %dx, %fs
  call   *%eax
  addl   $8, %esp
  pop    %fs
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

