nasm -f elf -o lib/kernel/print.o lib/kernel/print.s
gcc -m32 -I lib/kernel/ -c -o main.o main.c
ld -m elf_i386 -Ttext 0xc0001500 -e main -o kernel.bin main.o lib/kernel/print.o
dd if=./kernel.bin of=/home/cc/file/boches/bochs/bin/c.img bs=512 count=200 seek=9 conv=notrunc
