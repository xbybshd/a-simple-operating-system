#include "init.h"
#include "print.h"
#include "interrupt.h"
#include "../device/timer.h"
#include "memory.h"
#include "thread.h"
#include "console.h"
#include "keyboard.h"
#include "tss.h"
#include "syscall-init.h"
#include "ide.h"
#include "fs.h"

void init_all(){
    put_str("init_all\n");
    idt_init();
    mem_init();
    
    thread_init();
    timer_init();
    keyboard_init();

    console_init();
    tss_init();
    syscall_init();
    ide_init();
    filesys_init();
}