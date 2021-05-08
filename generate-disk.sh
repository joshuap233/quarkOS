#!/bin/bash

if [ ! -f "disk.img" ]; then
  dd if=/dev/zero of=disk.img bs=1M seek=1024 count=0
  mkfs -t ext2 -b 4096 disk.img
fi

