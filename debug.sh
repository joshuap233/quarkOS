#!/bin/zsh

qemu-system-i386                                 \
  -m 128                                         \
  -no-reboot                                     \
  -s -S                                          \
  -drive format=raw,media=disk,file=cmake-build-debug/quarkOS.iso,if=ide
