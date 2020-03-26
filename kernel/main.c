#include "print.h"
#include "init.h"
#include "debug.h"
#include "thread.h"
#include "interrupt.h"
#include "console.h"
#include "keyboard.h"
#include "ioqueue.h"
#include "process.h"
#include "syscall-init.h"
#include "syscall.h"
#include "stdio.h"
#include "memory.h"
#include "fs.h"
#include "string.h"
#include "dir.h"
#include "shell.h"
#include "list.h"

void k_thread_a(void *);
void k_thread_b(void *);
void u_prog_a(void);
void u_prog_b(void);
// int prog_a_pid = 0, prog_b_pid = 0;



int main(void){
    put_str("I am kernel\n");
    init_all();

         intr_enable();
    // sys_open("/file1", O_CREAT);
    // uint32_t fd1=sys_open("/file1", O_RDWR);
    // printf("fd:%d\n", fd1);
    // sys_write(fd1, "Hello,world\n", 12);
    // sys_write(fd1, "Hello,world\n", 12);
    // sys_write(fd1, "Hello,world\n", 12);
    // sys_close(fd1);


    uint32_t file_size=10116;
    uint32_t sec_cnt=DIV_ROUND_UP(file_size,512);
    struct disk* sda=&channels[0].devices[0];
    void* prog_buf=sys_malloc(file_size);
    //cls_screen();
    ide_read(sda,300,prog_buf,sec_cnt);
    printf("read\n");
    int32_t fd=sys_open("/prog_pipe",O_CREAT|O_RDWR);
    if(fd==-1){
        printf("unlink !\n");
        unlink("/prog_pipe");
        fd=sys_open("/prog_pipe",O_CREAT|O_RDWR);
    }
    if(fd!=-1){
        // printf("unlink !\n");
        // unlink("/prog_no_arg");
    
        printf("write\n");
        if(sys_write(fd,prog_buf,file_size)==-1){
                printf("file write error!\n");
                while(1);
        }
    }
    

    cls_screen();
   
    console_put_str("[rabbit@localhost /]$ ");
    thread_exit(running_thread(),true);
    while(1);




    return 0;
}


void init(void){
    uint32_t ret_pid=fork();
    if(ret_pid){
        int status;
        int child_pid;

        while(1){
            child_pid=wait(&status);
            printf("i am init, My pid is 1, I recive a child, It's pid is %d status is %d\n",child_pid,status);
        };
    }
    else{
        //printf("i am child, my pid is %d, ret pid is %d\n",getpid(),ret_pid);
        my_shell();
    }
    PANIC("init: should not be here");
    while(1);
}

void k_thread_a(void* arg){
    void *addr1 = sys_malloc(256);
    void *addr2 = sys_malloc(255);
    void *addr3 = sys_malloc(254);
    console_put_str(" thread_a malloc addr:0x");
    console_put_int((int)addr1);
    console_put_char(',');
    console_put_int((int)addr2);
    console_put_char(',');
    console_put_int((int)addr3);
    console_put_char('\n');
    int cpu_delay = 1000000;
    while(cpu_delay-->0);
    sys_free(addr1);
    sys_free(addr2);
    sys_free(addr3);
    while(1);
}

void k_thread_b(void* arg){
     void *addr1 = sys_malloc(256);
    void *addr2 = sys_malloc(255);
    void *addr3 = sys_malloc(254);
    console_put_str(" thread_b malloc addr:0x");
    console_put_int((int)addr1);
    console_put_char(',');
    console_put_int((int)addr2);
    console_put_char(',');
    console_put_int((int)addr3);
    console_put_char('\n');
    int cpu_delay = 1000000;
    while(cpu_delay-->0);
    sys_free(addr1);
    sys_free(addr2);
    sys_free(addr3);
    while(1);
}

void u_prog_a(void){
    void *addr1 = malloc(256);
    void *addr2 = malloc(255);
    void *addr3 = malloc(254);

    printf(" prog_a malloc addr:0x%x,0x%x,0x%x\n", (int)addr1, (int)addr2, (int)addr3);
    int cpu_delay = 100000;
    while(cpu_delay-->0);
    free(addr1);
    free(addr2);
    free(addr3);
    while(1)
        ;
}

void u_prog_b(void){
    void *addr1 = malloc(256);
    void *addr2 = malloc(255);
    void *addr3 = malloc(254);

    printf(" prog_b malloc addr:0x%x,0x%x,0x%x\n", (int)addr1, (int)addr2, (int)addr3);
    int cpu_delay = 100000;
    while(cpu_delay-->0);
    free(addr1);
    free(addr2);
    free(addr3);
    while(1)
        ;
}
