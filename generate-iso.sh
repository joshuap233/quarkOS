#!/bin/bash -e

# 检查是否符合 multiboot2 规范
if grub-file --is-x86-multiboot2 $1
then
    echo multiboot2 confirmed
  else
    echo the file is not multiboot
    exit 1
fi

pwd

cp $1 ../../isodir/boot
grub-mkrescue -o quarkOS.iso ../../isodir