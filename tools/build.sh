#!/bin/bash

bootsect=$1
setup=$2
IMAGE=$3

# Write bootsect (512 bytes, one sector) to stdout
[ ! -f "$bootsect" ] && echo "找不到二进制文件 bootsect" && exit -1
dd if=$bootsect bs=512 count=1 of=$IMAGE 2>&1 >/dev/null

# Write setup(4 * 512bytes, four sectors) to stdout
[ ! -f "$setup" ] && echo "找不到二进制文件 setup" && exit -1
dd if=$setup seek=1 bs=512 count=4 of=$IMAGE 2>&1 >/dev/null

