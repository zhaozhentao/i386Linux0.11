# 启动文件

.global _start

.equ BOOTSEG, 0x07c0		# original address of boot-sector

ljmp    $BOOTSEG, $_start
_start:

