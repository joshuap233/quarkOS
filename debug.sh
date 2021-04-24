#!/bin/zsh

if [ ! -f "disk.img" ]; then
  dd if=/dev/zero of=disk.img bs=1M seek=1024 count=0
  mkfs -t ext2 disk.img
fi



qemu-system-i386                                 \
  -m 128                                         \
  -no-reboot                                     \
  -s -S                                          \
  -drive format=raw,media=disk,file=disk.img,if=ide,index=0   \
  -drive format=raw,media=cdrom,file=cmake-build-debug/quarkOS.iso \
  -boot d
