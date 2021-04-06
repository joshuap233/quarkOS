#!/bin/zsh

qemu-system-i386                                 \
  -m 128                                         \
  -drive format=raw,media=disk,file=cmake-build-debug/quarkOS.iso,if=ide
