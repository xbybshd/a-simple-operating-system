BUILD_DIR=./build
ENTRY_POINT=0xc0001500
AS=nasm
CC=gcc
LD=ld
LIB= -I lib/ -I lib/kernel/ -I lib/user/ -I kernel/ -I device/ -I thread/ -I userprog/ -I fs/ -I shell/
ASFLAGS=-f elf
CFLAGS=-m32 -Wall $(LIB) -DNDEBUG -c -fno-builtin -fno-stack-protector -W -Wstrict-prototypes  -Wmissing-prototypes
LDFLAGS= -m elf_i386 -Ttext $(ENTRY_POINT) -e main -Map $(BUILD_DIR)/kernel.Map
OBJS=$(BUILD_DIR)/main.o $(BUILD_DIR)/init.o $(BUILD_DIR)/interrupt.o $(BUILD_DIR)/timer.o $(BUILD_DIR)/kernel.o \
	$(BUILD_DIR)/print.o $(BUILD_DIR)/debug.o $(BUILD_DIR)/list.o $(BUILD_DIR)/switch.o $(BUILD_DIR)/string.o $(BUILD_DIR)/bitmap.o $(BUILD_DIR)/memory.o $(BUILD_DIR)/thread.o \
	$(BUILD_DIR)/sync.o $(BUILD_DIR)/console.o $(BUILD_DIR)/ioqueue.o $(BUILD_DIR)/keyboard.o $(BUILD_DIR)/tss.o $(BUILD_DIR)/process.o $(BUILD_DIR)/syscall-init.o \
	 $(BUILD_DIR)/syscall.o $(BUILD_DIR)/stdio.o $(BUILD_DIR)/stdio-kernel.o $(BUILD_DIR)/ide.o $(BUILD_DIR)/inode.o $(BUILD_DIR)/dir.o $(BUILD_DIR)/file.o \
	 $(BUILD_DIR)/fs.o $(BUILD_DIR)/fork.o $(BUILD_DIR)/shell.o $(BUILD_DIR)/buildin_cmd.o $(BUILD_DIR)/exec.o $(BUILD_DIR)/wait_exit.o $(BUILD_DIR)/pipe.o

$(BUILD_DIR)/main.o: kernel/main.c lib/kernel/print.h lib/kernel/stdint.h kernel/init.h thread/thread.h kernel/interrupt.h device/ioqueue.h device/keyboard.h \
      userprog/syscall-init.h lib/user/syscall.h lib/stdio.h kernel/memory.h fs/fs.h lib/string.h fs/dir.h shell/shell.h lib/kernel/list.h
	$(CC) $(CFLAGS) $< -o $@
$(BUILD_DIR)/init.o :kernel/init.c kernel/init.h lib/kernel/print.h lib/kernel/stdint.h kernel/interrupt.h device/timer.h thread/thread.h device/console.h \
		device/keyboard.h userprog/tss.h kernel/memory.h userprog/syscall-init.h device/ide.h fs/fs.h
	$(CC) $(CFLAGS) $< -o $@
$(BUILD_DIR)/interrupt.o :kernel/interrupt.c kernel/interrupt.h lib/kernel/stdint.h kernel/global.h lib/kernel/io.h lib/kernel/print.h
	$(CC) $(CFLAGS) $< -o $@
$(BUILD_DIR)/list.o :lib/kernel/list.c lib/kernel/list.h lib/kernel/stdint.h kernel/global.h kernel/interrupt.h lib/kernel/print.h
	$(CC) $(CFLAGS) $< -o $@
$(BUILD_DIR)/debug.o: kernel/debug.c kernel/debug.h lib/kernel/print.h lib/kernel/stdint.h kernel/interrupt.h
	$(CC) $(CFLAGS) $< -o $@
$(BUILD_DIR)/string.o: lib/string.c lib/string.h lib/kernel/stdint.h kernel/global.h kernel/debug.h
	$(CC) $(CFLAGS) $< -o $@
$(BUILD_DIR)/bitmap.o: lib/kernel/bitmap.c kernel/global.h lib/kernel/bitmap.h lib/kernel/stdint.h lib/string.h kernel/interrupt.h lib/kernel/print.h kernel/debug.h
	$(CC) $(CFLAGS) $< -o $@
$(BUILD_DIR)/memory.o: kernel/memory.c kernel/memory.h lib/kernel/bitmap.h lib/string.h kernel/global.h kernel/interrupt.h kernel/debug.h lib/kernel/stdint.h lib/kernel/print.h \
           lib/kernel/list.h kernel/interrupt.h device/keyboard.h device/ioqueue.h thread/thread.h
	$(CC) $(CFLAGS) $< -o $@
$(BUILD_DIR)/sync.o: thread/sync.c thread/sync.h thread/thread.h lib/kernel/bitmap.h lib/string.h kernel/global.h kernel/interrupt.h kernel/debug.h lib/kernel/stdint.h lib/kernel/list.h
	$(CC) $(CFLAGS) $< -o $@
$(BUILD_DIR)/thread.o: thread/thread.c thread/thread.h lib/kernel/bitmap.h lib/string.h kernel/global.h kernel/interrupt.h kernel/debug.h lib/kernel/stdint.h kernel/memory.h lib/kernel/print.h thread/sync.h \
lib/kernel/list.h
	$(CC) $(CFLAGS) $< -o $@
$(BUILD_DIR)/timer.o: device/timer.c device/timer.h lib/kernel/stdint.h lib/kernel/io.h lib/kernel/print.h thread/thread.h kernel/debug.h kernel/interrupt.h
	$(CC) $(CFLAGS) $< -o $@
$(BUILD_DIR)/console.o: device/console.c device/console.h kernel/global.h kernel/debug.h lib/kernel/stdint.h lib/kernel/print.h thread/sync.h thread/thread.h
	$(CC) $(CFLAGS) $< -o $@
$(BUILD_DIR)/ioqueue.o: device/ioqueue.c device/ioqueue.h kernel/global.h kernel/interrupt.h kernel/debug.h
	$(CC) $(CFLAGS) $< -o $@
$(BUILD_DIR)/keyboard.o: device/keyboard.c device/keyboard.h kernel/global.h lib/kernel/print.h kernel/interrupt.h lib/kernel/io.h
	$(CC) $(CFLAGS) $< -o $@
$(BUILD_DIR)/tss.o: userprog/tss.c userprog/tss.h kernel/global.h lib/kernel/print.h lib/string.h lib/kernel/stdint.h thread/thread.h
	$(CC) $(CFLAGS) $< -o $@
$(BUILD_DIR)/process.o: userprog/process.c userprog/process.h userprog/tss.h kernel/global.h lib/kernel/print.h lib/string.h lib/kernel/stdint.h thread/thread.h \
        kernel/memory.h kernel/interrupt.h kernel/debug.h lib/kernel/list.h device/console.h
	$(CC) $(CFLAGS) $< -o $@
$(BUILD_DIR)/syscall-init.o: userprog/syscall-init.c userprog/syscall-init.h userprog/tss.h kernel/global.h lib/kernel/print.h lib/string.h lib/kernel/stdint.h thread/thread.h \
        kernel/memory.h kernel/interrupt.h device/console.h fs/fs.h userprog/fork.h
	$(CC) $(CFLAGS) $< -o $@
$(BUILD_DIR)/syscall.o: lib/user/syscall.c lib/user/syscall.h lib/kernel/stdint.h 
	$(CC) $(CFLAGS) $< -o $@
$(BUILD_DIR)/stdio.o: lib/stdio.c lib/stdio.h kernel/interrupt.h kernel/global.h lib/string.h lib/kernel/stdint.h lib/kernel/print.h lib/user/syscall.h
	$(CC) $(CFLAGS) $< -o $@
$(BUILD_DIR)/stdio-kernel.o: lib/kernel/stdio-kernel.c lib/kernel/stdio-kernel.h lib/kernel/print.h lib/stdio.h device/console.h \
kernel/global.h
	$(CC) $(CFLAGS) $< -o $@
$(BUILD_DIR)/ide.o: device/ide.c device/ide.h thread/sync.h lib/kernel/io.h lib/stdio.h lib/kernel/stdio-kernel.h kernel/interrupt.h \
kernel/debug.h kernel/memory.h device/console.h device/timer.h lib/string.h lib/kernel/list.h fs/super_block.h
	$(CC) $(CFLAGS) $< -o $@
$(BUILD_DIR)/inode.o: fs/inode.c fs/inode.h kernel/memory.h kernel/debug.h fs/inode.h fs/super_block.h kernel/memory.h kernel/debug.h kernel/interrupt.h\
lib/kernel/list.h fs/super_block.h lib/string.h fs/fs.c fs/file.h
	$(CC) $(CFLAGS) $< -o $@
$(BUILD_DIR)/dir.o: fs/dir.c fs/dir.h fs/inode.h kernel/memory.h kernel/debug.h fs/inode.h fs/super_block.h kernel/memory.h kernel/debug.h kernel/interrupt.h\
lib/kernel/list.h fs/super_block.h lib/string.h fs/fs.c fs/file.h kernel/global.h device/ide.h
	$(CC) $(CFLAGS) $< -o $@
$(BUILD_DIR)/file.o: fs/file.c fs/file.h fs/inode.h kernel/memory.h kernel/debug.h fs/inode.h fs/super_block.h kernel/memory.h kernel/debug.h kernel/interrupt.h\
lib/kernel/list.h fs/super_block.h lib/string.h fs/fs.c fs/file.h kernel/global.h thread/thread.h device/ioqueue.h thread/thread.h lib/kernel/list.h
	$(CC) $(CFLAGS) $< -o $@
$(BUILD_DIR)/fs.o: fs/fs.c fs/fs.h device/console.h lib/string.h lib/kernel/list.h lib/kernel/stdio-kernel.h kernel/memory.h kernel/debug.h \
device/keyboard.h fs/dir.h fs/inode.h fs/super_block.h thread/sync.h 
	$(CC) $(CFLAGS) $< -o $@
$(BUILD_DIR)/fork.o: userprog/fork.c userprog/fork.h userprog/process.h kernel/memory.h kernel/interrupt.h kernel/debug.h thread/thread.h \
lib/string.h fs/file.h fs/inode.h
	$(CC) $(CFLAGS) $< -o $@
$(BUILD_DIR)/shell.o: shell/shell.c shell/shell.h lib/kernel/stdint.h fs/file.h lib/user/syscall.h lib/stdio.h kernel/global.h kernel/debug.h lib/string.h \
shell/buildin_cmd.h lib/kernel/list.h
	$(CC) $(CFLAGS) $< -o $@
$(BUILD_DIR)/buildin_cmd.o: shell/buildin_cmd.c shell/buildin_cmd.h lib/user/syscall.h lib/string.h lib/stdio.h fs/fs.h fs/dir.h shell/shell.h lib/kernel/stdint.h \
kernel/debug.h
	$(CC) $(CFLAGS) $< -o $@
$(BUILD_DIR)/exec.o: userprog/exec.c userprog/exec.h thread/thread.h  \
    	lib/kernel/list.h kernel/global.h lib/kernel/bitmap.h kernel/memory.h \
     	lib/kernel/stdio-kernel.h fs/fs.h lib/string.h lib/kernel/stdint.h \
		 lib/kernel/list.h
	$(CC) $(CFLAGS) $< -o $@
$(BUILD_DIR)/wait_exit.o: userprog/wait_exit.c userprog/wait_exit.h \
    	thread/thread.h lib/kernel/stdint.h lib/kernel/list.h \
     	kernel/global.h lib/kernel/bitmap.h kernel/memory.h kernel/debug.h \
      	thread/thread.h lib/kernel/stdio-kernel.h
	$(CC) $(CFLAGS) $< -o $@
$(BUILD_DIR)/pipe.o: shell/pipe.c shell/pipe.h lib/kernel/stdint.h kernel/memory.h \
    	lib/kernel/bitmap.h kernel/global.h lib/kernel/list.h fs/fs.h fs/file.h \
     	device/ide.h thread/sync.h thread/thread.h fs/dir.h fs/inode.h fs/fs.h \
      	device/ioqueue.h thread/thread.h
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/kernel.o: kernel/kernel.s
	$(AS) $(ASFLAGS) $< -o $@
$(BUILD_DIR)/print.o: lib/kernel/print.s
	$(AS) $(ASFLAGS) $< -o $@
$(BUILD_DIR)/switch.o: thread/switch.s
	$(AS) $(ASFLAGS) $< -o $@

$(BUILD_DIR)/kernel.bin:$(OBJS)
	$(LD) $(LDFLAGS) $^ -o $@

.PHONY:mkdir hd clean all
mkdir:
	# if[! -d $(BUILD_DIR)];then mkdir $(BUILD_DIR);fi
hd:
	dd if=$(BUILD_DIR)/kernel.bin of=/home/cc/file/boches/bochs/bin/c.img bs=512 count=200 seek=9 conv=notrunc
clean:
	cd $(BUILD_DIR) && rm -f ./*;
build: $(BUILD_DIR)/kernel.bin
all: mkdir build hd