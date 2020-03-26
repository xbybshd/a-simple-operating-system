#！/bin/bash
# chmod 777 ./comp.sh
# nasm -I include/ -o mbr_4.3.bin mbr_4.3.s
# dd if=./mbr_4.3.bin of=/home/cc/file/boches/bochs/bin/c.img bs=512 count=1 conv=notrunc
nasm -I include/ -o mbr_4.3.bin mbr_4.3.s
dd if=./mbr_4.3.bin of=/home/cc/file/boches/bochs/bin/c.img bs=512 count=1 conv=notrunc
nasm -I include/ -o loader.bin loader.s
dd if=./loader.bin of=/home/cc/file/boches/bochs/bin/c.img bs=512 count=5 seek=2 conv=notrunc