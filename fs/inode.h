#ifndef __FS_INODE_H__
#define __FS_INODE_H__
#include "stdint.h"
#include "list.h"
#include "ide.h"

struct inode{
    uint32_t i_no;

    uint32_t i_size;

    uint32_t i_open_cnts;
    bool write_deny;

    uint32_t i_sectors[13];//12个是直接的磁盘扇区号，13个也是磁盘扇区号，不过里面存储着１２８个一级扇区号
    struct list_elem inode_tag;
};
void inode_sync(struct partition *part, struct inode *inode, void *io_buf);
struct inode* inode_open(struct partition *part, uint32_t inode_no);
void inode_close(struct inode *inode);
void inode_delete(struct partition *part, uint32_t inode_no, void *io_buf);
void inode_release(struct partition *part, uint32_t inode_no);
void inode_init(uint32_t inode_no, struct inode *new_inode);

#endif