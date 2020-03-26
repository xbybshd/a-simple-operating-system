#！/bin/bash

nasm -I include/ -o mbr_4.3.bin mbr_4.3.s
dd if=./mbr_4.3.bin of=/home/cc/file/boches/bochs/bin/c.img bs=512 count=1 conv=notrunc
