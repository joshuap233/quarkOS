#!/bin/bash

rm -f disk.img
dd if=/dev/zero of=disk.img bs=1M seek=1024 count=0
mkfs -t ext2 -b 4096 -I 256 disk.img

mkdir -p /tmp/disk
sudo mount disk.img /tmp/disk
sudo mkdir /tmp/disk/bin
sudo cp sh /tmp/disk/bin
sudo umount /tmp/disk

