#include "inode.h"
#include "fs.h"
#include "file.h"
#include "global.h"
#include "debug.h"
#include "memory.h"
#include "interrupt.h"
#include "list.h"
#include "stdio-kernel.h"
#include "string.h"
#include "super_block.h"

struct inode_position{
    bool two_sec;
    uint32_t sec_lba;
    uint32_t off_size;
};

//获取inode节点的扇区地址,inode节点是存储在磁盘的inode数组里面的
static void inode_locate(struct partition* part,uint32_t inode_no,struct inode_position* inode_pos){
    ASSERT(inode_no < 4096);
    uint32_t inode_table_lba = part->sb->inode_table_lba;

    uint32_t inode_size = sizeof(struct inode);
    uint32_t off_size = inode_no * inode_size;
    uint32_t off_sec = off_size / 512;
    uint32_t off_size_in_sec = off_size % 512;

    uint32_t left_in_sec = 512 - off_size_in_sec;
    if(left_in_sec<inode_size){
        inode_pos->two_sec = true;
    }
    else{
        inode_pos->two_sec = false;
    }
    inode_pos->sec_lba = inode_table_lba + off_sec;
    inode_pos->off_size = off_size_in_sec;
}

void inode_sync(struct partition* part,struct inode* inode,void* io_buf){
    uint8_t inode_no = inode->i_no;
    struct inode_position inode_pos;
    inode_locate(part, inode_no, &inode_pos);
    ASSERT(inode_pos.sec_lba <= (part->start_lba + part->sec_cnt));

    struct inode pure_inode;
    memcpy(&pure_inode, inode, sizeof(struct inode));

    pure_inode.i_open_cnts = 0;
    pure_inode.write_deny = false;
    pure_inode.inode_tag.prev = pure_inode.inode_tag.next = NULL;//inode写入硬盘后，与运行时相关的元素应该清零或者回归原样

    char *inode_buf = (char *)io_buf;
    if(inode_pos.two_sec){
        printf("read2");
        ide_read(part->my_disk, inode_pos.sec_lba, inode_buf, 2);//先把磁盘里面两个扇区读出来，读到缓冲区内，再写入缓冲区，再将整个缓冲区写入扇区
        memcpy((inode_buf + inode_pos.off_size), &pure_inode, sizeof(struct inode));
        ide_write(part->my_disk, inode_pos.sec_lba, inode_buf, 2);
    }
    else{
        printf("read3");
        ide_read(part->my_disk, inode_pos.sec_lba, inode_buf, 1);
        memcpy((inode_buf + inode_pos.off_size), &pure_inode, sizeof(struct inode));
        ide_write(part->my_disk, inode_pos.sec_lba, inode_buf, 1);

    }
}

//根据inode节点号返回inode节点的数据结构
struct inode* inode_open(struct partition* part, uint32_t inode_no){
    struct list_elem *elem = part->open_inodes.head.next;
    struct inode *inode_found;
    while(elem!=&part->open_inodes.tail){//在内存中的inode链表里面找
        inode_found = elem2entry(struct inode, inode_tag, elem);
        if(inode_found->i_no==inode_no){
            inode_found->i_open_cnts++;
            return inode_found;
        }
        elem = elem->next;
    }

    struct inode_position inode_pos;
    inode_locate(part, inode_no, &inode_pos);

    struct task_struct *cur = running_thread();
    uint32_t *cur_pagedir_bak = cur->pgdir;
    cur->pgdir = NULL;//堆中分配内存如果进程控制块中用户进程页表存在就在用户堆中分配，否则在内核进程

    inode_found = (struct inode *)sys_malloc(sizeof(struct inode));
    cur->pgdir = cur_pagedir_bak;

    char *inode_buf;
    if(inode_pos.two_sec){
        inode_buf = (char *)sys_malloc(1024);
      //  printf("inode open 1\n");
        ide_read(part->my_disk, inode_pos.sec_lba, inode_buf, 2);
    }
    else{
        inode_buf = (char *)sys_malloc(512);
     //   printf("inode open 2\n");
     //   printk("inode_pos.sec_lba :%d\n",inode_pos.sec_lba);
        ide_read(part->my_disk, inode_pos.sec_lba, inode_buf, 1);        
    }

    memcpy(inode_found, inode_buf + inode_pos.off_size, sizeof(struct inode));

    list_push(&part->open_inodes, &inode_found->inode_tag);
    inode_found->i_open_cnts = 1;//打开数+1
    sys_free(inode_buf);
    return inode_found;
}

//关闭inode减少打开数
void inode_close(struct inode* inode){
    enum intr_status old_status = intr_disable();
    if(--inode->i_open_cnts==0){
        list_remove(&inode->inode_tag);//放inode节点的时候也要放在内核堆中，所以换页表
        struct task_struct *cur = running_thread();
        uint32_t* cur_pagedir_bak = cur->pgdir;
        cur->pgdir = NULL;
        sys_free(inode);
        cur->pgdir = cur_pagedir_bak;
    }
    intr_set_status(old_status);
}


void inode_delete(struct partition* part,uint32_t inode_no,void* io_buf){//仅仅是shanquinode,而不是回收inode所带的扇区
    ASSERT(inode_no < 4096);
    struct inode_position inode_pos;
    inode_locate(part, inode_no, &inode_pos);

    ASSERT(inode_pos.sec_lba <= (part->start_lba + part->sec_cnt));
    char *inode_buf = (char *)io_buf;
    if(inode_pos.two_sec){
        ide_read(part->my_disk, inode_pos.sec_lba, inode_buf, 2);
        memset((inode_buf + inode_pos.off_size), 0, sizeof(struct inode));
        ide_write(part->my_disk, inode_pos.sec_lba, inode_buf, 2);
    }
    else{
        ide_read(part->my_disk, inode_pos.sec_lba, inode_buf, 1);
        memset((inode_buf + inode_pos.off_size), 0, sizeof(struct inode));
        ide_write(part->my_disk, inode_pos.sec_lba, inode_buf, 1);  
    }
}

void inode_release(struct partition* part,uint32_t inode_no){//把这个inode_no节点所指向的扇区全部回收
    struct inode *inode_to_del = inode_open(part, inode_no);
    ASSERT(inode_to_del->i_no == inode_no);
    uint8_t block_idx = 0, block_cnt = 12;
    uint32_t block_bitmap_idx;
    uint32_t all_blocks[140] = {0};
    while(block_idx<12){
        all_blocks[block_idx] = inode_to_del->i_sectors[block_idx];//读取直接块
        block_idx++;
    }
    if(inode_to_del->i_sectors[12]!=0){
        ide_read(part->my_disk, inode_to_del->i_sectors[12], all_blocks + 12, 1);//读取所有间接块
        block_cnt = 140;

        block_bitmap_idx = inode_to_del->i_sectors[12] - part->sb->data_start_lba;
        ASSERT(block_bitmap_idx > 0);
        bitmap_set(&part->block_bitmap, block_bitmap_idx, 0);//回收一级间接块的扇区
        bitmap_sync(cur_part, block_bitmap_idx, BLOCK_BITMAP);
    }

    block_idx = 0;
    while(block_idx<block_cnt){
        if(all_blocks[block_idx]!=0){
            block_bitmap_idx = 0;
            block_bitmap_idx = all_blocks[block_idx] - part->sb->data_start_lba;//回收所有的直接块和间接块
            ASSERT(block_bitmap_idx > 0);
            bitmap_set(&part->block_bitmap, block_bitmap_idx, 0);
            bitmap_sync(cur_part, block_bitmap_idx, BLOCK_BITMAP);
        }
        block_idx++;
    }
    bitmap_set(&part->inode_bitmap, inode_no, 0);
    bitmap_sync(cur_part, inode_no, INODE_BITMAP);

    void *io_buf = sys_malloc(1024);//回收inode数据没有必要清零，只要在位图中置０即可，清零是为了方便调试
    inode_delete(part, inode_no, io_buf);
    sys_free(io_buf);

    inode_close(inode_to_del);
}



void inode_init(uint32_t inode_no,struct inode* new_inode){
    new_inode->i_no = inode_no;
    new_inode->i_size = 0;
    new_inode->i_open_cnts = 0;
    new_inode->write_deny = false;

    uint8_t sec_idx = 0;
    while(sec_idx<13){
        new_inode->i_sectors[sec_idx] = 0;
        sec_idx++;
    }
}
