#ifndef __FS_FS_H__
#define __FS_FS_H__
#include "stdint.h"

#define MAX_FILES_PER_PART 4096
#define BITS_PER_SECTOR 4096
#define SECTOR_SIZE 512
#define BLOCK_SIZE SECTOR_SIZE 
#define MAX_PATH_LEN 512

enum file_types
{
    FT_UNKNOWN,
    FT_REGULAR,
    FT_DIRECTORY
};

enum oflags
{
    O_RDONLY,
    O_WRONLY,
    O_RDWR,
    O_CREAT = 4
};

enum whence
{
    SEEK_SET = 1,
    SEEK_CUR,
    SEEK_END
};
struct path_search_record
{
    char searched_path[MAX_PATH_LEN];
    struct dir *parent_dir;
    enum file_types file_type;
};

struct stat{
    uint32_t st_ino;
    uint32_t st_size;
    enum file_types st_filetype;
};

struct partition *cur_part;
int32_t path_depth_cnt(char *pathname);
char* path_parse(char* pathname,char* name_store);
int32_t sys_open(const char *pathname, uint8_t flags);
uint32_t fd_local2global(uint32_t local_fd);
int32_t sys_close(int32_t fd);
int32_t sys_write(int32_t fd, const void *buf, uint32_t count);
int32_t sys_unlink(const char *pathname);
int32_t sys_read(int32_t fd, void *buf, uint32_t count);
void sys_putchar(char char_asci);
int32_t sys_lseek(int32_t fd, int32_t offset, uint8_t whence);
int32_t sys_mkdir(const char *pathname);
struct dir *sys_opendir(const char *name);
int32_t sys_closedir(struct dir *dir);
struct dir_entry *sys_readdir(struct dir *dir);
void sys_rewinddir(struct dir *dir);
int32_t sys_rmdir(const char *pathname);
char *sys_getcwd(char *buf, uint32_t size);
int32_t sys_chdir(const char *path);
int32_t sys_stat(const char *path, struct stat *buf);
void filesys_init();
#endif