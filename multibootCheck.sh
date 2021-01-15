#!/bin/bash
# 检查是否符合 multiboot2 规范
if grub-file --is-x86-multiboot2 build/quarkOS.bin
then
    echo multiboot2 confirmed
  else
    echo the file is not multiboot
    exit 1
fi