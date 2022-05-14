#!/bin/bash
 
sudo dd if=/dev/zero of=ramdisk_rootfs_usb_update.ext4 bs=1M count=20
 
sudo mkfs.ext4 -F ramdisk_rootfs_usb_update.ext4
 
sudo mount -t ext4 ramdisk_rootfs_usb_update.ext4 ./mntrd
 
sleep 1

sudo cp ramdisk/* ./mntrd -r -d

sudo mknod ./mntrd/dev/console c 5 1
sudo mknod ./mntrd/dev/null c 1 3
 
sudo umount ./mntrd
 
sudo gzip --best -c ramdisk_rootfs_usb_update.ext4 > ramdisk_rootfs_usb_update.ext4.gz
 
sudo rm ramdisk_rootfs_usb_update.ext4
