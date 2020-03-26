#！/bin/bash
gcc -m32 -c -o main.o main.c
ld -m elf_i386 main.o -Ttext 0xc0001500 -o kernel.bin
dd if=./kernel.bin of=/home/cc/file/boches/bochs/bin/c.img bs=512 count=200 seek=9 conv=notrunc