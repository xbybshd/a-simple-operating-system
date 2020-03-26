#！/bin/bash

#mbr
nasm -I include/ -o mbr_4.3.bin mbr_4.3.s
dd if=./mbr_4.3.bin of=/home/cc/file/boches/bochs/bin/c.img bs=512 count=1 conv=notrunc
#loader
nasm -I include/ -o loader.bin loader.s
dd if=./loader.bin of=/home/cc/file/boches/bochs/bin/c.img bs=512 count=5 seek=2 conv=notrunc

# gcc -m32 -I lib/kernel -c -o bulid/timer.o device/timer.c 
gcc -m32 -I lib/kernel/ -I lib/ -I kernel/ -c -fno-builtin -o bulid/main.o kernel/main.c
nasm -f elf -o bulid/print.o lib/kernel/print.s
nasm -f elf -o bulid/kernel.o kernel/kernel.s
gcc -m32 -I lib/kernel/ -I lib/ -I kernel/ -c -fno-builtin -fno-stack-protector -o bulid/interrupt.o kernel/interrupt.c
gcc -m32 -I lib/kernel/ -I lib/ -I kernel/ -c -fno-builtin -o bulid/init.o kernel/init.c
ld -m elf_i386 -Ttext 0xc0001500 -e main -o bulid/kernel.bin bulid/main.o bulid/init.o \
 bulid/interrupt.o bulid/print.o bulid/kernel.o 
dd if=bulid/kernel.bin of=/home/cc/file/boches/bochs/bin/c.img bs=512 count=200 seek=9 conv=notrunc