#ifndef __FS_DIR_H__
#define __FS_DIR_H__

#include "super_block.h"
#include "fs.h"
#include "ide.h"
#define MAX_FILE_NAME_LEN 16

struct dir{//在程序使用的时候内存里建立的数据结构
    struct inode *inode;
    uint32_t dir_pos;
    uint8_t dir_buf[512];
};

struct dir_entry{//存储在硬盘上目录文件里的目录项
    char filename[MAX_FILE_NAME_LEN];
    uint32_t i_no;
    enum file_types f_type;
};

extern struct dir root_dir;
void open_root_dir(struct partition *part);
struct dir *dir_open(struct partition *part, uint32_t inode_no);
bool search_dir_entry(struct partition *part, struct dir *pdir, const char *name, struct dir_entry *dir_e);
void dir_close(struct dir *dir);
void create_dir_entry(char *filename, uint32_t inode_no, uint8_t file_type, struct dir_entry *p_de);
bool sync_dir_entry(struct dir *parent_dir, struct dir_entry *p_de, void *io_buf);
bool delete_dir_entry(struct partition *part, struct dir *pdir, uint32_t inode_no, void *io_buf);
struct dir_entry *dir_read(struct dir *dir);
bool dir_is_empty(struct dir *dir);
int32_t dir_remove(struct dir *parent_dir, struct dir *child_dir);
#endif