#!/bin/bash

if [ ! -f "disk.img" ]; then
  dd if=/dev/zero of=disk.img bs=1M seek=1024 count=0
  mkfs -t ext2 -b 4096 -I 256 disk.img

  # 复制用户空间代码到 bin 目录
  mkdir -p /tmp/disk
  sudo mount disk.img /tmp/disk
  sudo mkdir -p /tmp/disk/bin
  sudo cp cmake-build-debug/userspace/sh /tmp/disk/bin
  sudo umount /tmp/disk
fi

