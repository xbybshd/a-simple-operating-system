#include "file.h"
#include "fs.h"
#include "super_block.h"
#include "inode.h"
#include "stdio-kernel.h"
#include "memory.h"
#include "debug.h"
#include "interrupt.h"
#include "string.h"
#include "thread.h"
#include "global.h"
#include "ioqueue.h"
#include "thread.h"
#include "list.h"

struct file file_table[MAX_FILE_OPEN];

//全局文件表中获取一个空位
int32_t get_free_slot_in_global(void){
    uint32_t fd_idx = 3;
    while(fd_idx<MAX_FILE_OPEN){
        
        if (file_table[fd_idx].fd_inode == NULL)
        {
            break;
        }
        fd_idx++;
    }
    if(fd_idx==MAX_FILE_OPEN){
        printk("exceed max open files\n");
        return -1;
    }
    return fd_idx;
}


//将全局文件表的下标，安装到进程线程自己的描述符表中
int32_t pcb_fd_install(int32_t global_fd_idx){
    struct task_struct *cur = running_thread();
    uint8_t local_fd_idx = 3;
    while(local_fd_idx<MAX_FILES_OPEN_PER_PROC){
        if(cur->fd_table[local_fd_idx]==-1){
            cur->fd_table[local_fd_idx] = global_fd_idx;
            break;
        }
        local_fd_idx++;
    }
    // printk("lfd: %d \n",local_fd_idx);
    if(local_fd_idx==MAX_FILES_OPEN_PER_PROC){
        printk("exceed max open files_per_proc\n");
        return -1;
    }
    return local_fd_idx;
}

//在分区inode位图中找到一个空的，分配一个inode节点号，并返回
int32_t inode_bitmap_alloc(struct partition* part){
    int32_t bit_idx = bitmap_scan(&part->inode_bitmap, 1);
    if(bit_idx==-1){
        return -1;
    }
    bitmap_set(&part->inode_bitmap, bit_idx, 1);
    return bit_idx;
}

int32_t block_bitmap_alloc(struct partition* part){
    int32_t bit_idx = bitmap_scan(&part->block_bitmap, 1);
    if(bit_idx==-1){
        return -1;
    }
    bitmap_set(&part->block_bitmap, bit_idx, 1);
    return (part->sb->data_start_lba + bit_idx);//返回第x个空闲扇区
}

void bitmap_sync(struct partition *part, uint32_t bit_idx, uint8_t btmp){
    uint32_t off_sec = bit_idx / 4096;
    uint32_t off_size = off_sec * BLOCK_SIZE;
    uint32_t sec_lba;
    uint8_t *bitmap_off;

    switch (btmp){
        case INODE_BITMAP:
            sec_lba = part->sb->inode_bitmap_lba + off_sec;//此位图索引在磁盘位图的扇区位置
            bitmap_off = part->inode_bitmap.bits + off_size;//此位图索引在内存位图的位置
            break;
        
        case BLOCK_BITMAP:
            sec_lba = part->sb->block_bitmap_lba + off_sec;
            bitmap_off = part->block_bitmap.bits + off_size;
            break;
        }
        ide_write(part->my_disk, sec_lba, bitmap_off, 1);
}

int32_t file_create(struct dir* parent_dir,char* filename,uint8_t flag){
    void *io_buf = sys_malloc(1024);
    if(io_buf==NULL){
        printk("in file_creat: sys_malloc for io_buf failed\n");
        return -1;
    }
    uint8_t rollback_step = 0;

    int32_t inode_no = inode_bitmap_alloc(cur_part);
    if (inode_no == -1){
        printk("in file_creat: allocate inode failed\n");
        return -1;
    }

    struct inode *new_file_inode = (struct inode *)sys_malloc(sizeof(struct inode));
    if(new_file_inode==NULL){
        printk("file_create:sys_malloc for inode failed\n");
        rollback_step = 1;
        goto rollback;
    }
    inode_init(inode_no, new_file_inode);

    int fd_idx = get_free_slot_in_global();
    if(fd_idx==-1){
        printk("exceed max open files!!!!\n");
        rollback_step = 2;
        goto rollback;
    }

    file_table[fd_idx].fd_inode = new_file_inode;
    file_table[fd_idx].fd_pos = 0;
    file_table[fd_idx].fd_flag = flag;
    file_table[fd_idx].fd_inode->write_deny = false;

    struct dir_entry new_dir_entry;
    memset(&new_dir_entry, 0, sizeof(struct dir_entry));
    //将新文件填入新的目录项
    create_dir_entry(filename, inode_no, FT_REGULAR, &new_dir_entry);//目录文件里面每一个目录项要么包含此目录底下一个文件，要么包含一个子目录

    if(!sync_dir_entry(parent_dir,&new_dir_entry,io_buf)){
        printk("sync dir_entry to disk failed\n");
        rollback_step = 3;
        goto rollback;
    }

    memset(io_buf, 0, 1024);
    inode_sync(cur_part, parent_dir->inode, io_buf);

    memset(io_buf, 0, 1024);
    printf("create1\n");
    inode_sync(cur_part, new_file_inode, io_buf);

    bitmap_sync(cur_part, inode_no, INODE_BITMAP);

    list_push(&cur_part->open_inodes, &new_file_inode->inode_tag);
    new_file_inode->i_open_cnts = 1;
    sys_free(io_buf);
    printf("create2\n");
    return pcb_fd_install(fd_idx);

rollback:
    switch (rollback_step)
    {
    case 3:
        memset(&file_table[fd_idx], 0, sizeof(struct file));
    case 2:
        sys_free(new_file_inode);
    case 1:
        bitmap_set(&cur_part->inode_bitmap, inode_no, 0);
        break;
    }
    sys_free(io_buf);
    return -1;
}

int32_t file_open(uint32_t inode_no,uint8_t flag){
    int fd_idx = get_free_slot_in_global();
    if(fd_idx==-1){
        printk("exceed mx open files\n");
        return -1;
    }
    file_table[fd_idx].fd_inode = inode_open(cur_part, inode_no);
    file_table[fd_idx].fd_pos = 0;
    file_table[fd_idx].fd_flag = flag;
    bool *write_deny = &file_table[fd_idx].fd_inode->write_deny;

    if(flag&O_WRONLY||flag&O_RDWR){
        enum intr_status old_status = intr_disable();
        if(!(*write_deny)){
            *write_deny = true;
            intr_set_status(old_status);
        }
        else{
            intr_set_status(old_status);
            printk("file can't be write now,try again later\n");
            return -1;
        }
    }
    return pcb_fd_install(fd_idx);
}


int32_t file_write(struct file* file,const void* buf,uint32_t count){
    if((file->fd_inode->i_size+count)>(BLOCK_SIZE*140)){
        printk("exceed max file_size 71680 bytes, write file failed\n");
        return -1;
    }
    uint8_t *io_buf = sys_malloc(512);
    if(io_buf==NULL){
        printk("file_write: sys_malloc for io_buf failed\n");
        return -1;
    }
    uint32_t *all_blocks = (uint32_t *)sys_malloc(BLOCK_SIZE + 48);
    if(all_blocks==NULL){
        printk("file_write: sys_malloc for all_blocks failed\n");
        return -1;
    }

    const uint8_t *src = buf;
    uint32_t bytes_written = 0;
    uint32_t size_left = count;
    int32_t block_lba = -1;
    uint32_t block_bitmap_idx = 0;

    uint32_t sec_idx; //用来索引扇区
    uint32_t sec_lba;//扇区地址
    uint32_t sec_off_bytes;//扇区内字节偏移量
    uint32_t sec_left_bytes;//扇区内剩余字节量
    uint32_t chunk_size;//每次写入硬盘的数据块大小
    int32_t indirect_block_table;//获取一级间接表地址
    uint32_t block_idx;//块索引

    //判断是不是第一次写文件，是的话就分配一个块
    if(file->fd_inode->i_sectors[0]==0){
        block_lba = block_bitmap_alloc(cur_part);
        if(block_lba==-1){
            printk("file write: block_bitmap_alloc failed\n");
            return -1;
        }
        file->fd_inode->i_sectors[0] = block_lba;

        block_bitmap_idx = block_lba - cur_part->sb->data_start_lba;
        ASSERT(block_bitmap_idx != 0);
        bitmap_sync(cur_part, block_bitmap_idx, BLOCK_BITMAP);
    }
    uint32_t file_has_used_blocks = file->fd_inode->i_size / BLOCK_SIZE + 1;
    uint32_t file_will_use_blocks = (file->fd_inode->i_size + count) / BLOCK_SIZE + 1;
    ASSERT(file_will_use_blocks <= 140);

    uint32_t add_blocks = file_will_use_blocks - file_has_used_blocks;
    if(add_blocks==0){//只写入同一个扇区
        if(file_will_use_blocks<=12){//写入的扇区小于１２
            block_idx = file_has_used_blocks - 1;
            all_blocks[block_idx] = file->fd_inode->i_sectors[block_idx];
        }
        else{//写入的扇区在间接块里
            ASSERT(file->fd_inode->i_sectors[12] != 0);
            indirect_block_table = file->fd_inode->i_sectors[12];
            //printk("write1\n");
            ide_read(cur_part->my_disk, indirect_block_table, all_blocks + 12, 1);
        }
    }else{//写入多个扇区
        if(file_will_use_blocks<=12){//写入的不超过１２个块
            block_idx = file_has_used_blocks - 1;
            ASSERT(file->fd_inode->i_sectors[block_idx] != 0);
            all_blocks[block_idx] = file->fd_inode->i_sectors[block_idx];

            block_idx = file_has_used_blocks;
            while(block_idx<file_will_use_blocks){
                block_lba = block_bitmap_alloc(cur_part);
                if(block_lba==-1){
                    printk("file_write: block_bitmap_alloc for situation 1 failed\n");
                    return -1;
                }
                ASSERT(file->fd_inode->i_sectors[block_idx] == 0);
                file->fd_inode->i_sectors[block_idx] = all_blocks[block_idx] = block_lba;

                block_bitmap_idx = block_lba - cur_part->sb->data_start_lba;//每分配一个扇区，就进行同步
                bitmap_sync(cur_part, block_bitmap_idx, BLOCK_BITMAP);
                block_idx++;
            }
        }
        else if(file_has_used_blocks<=12&&file_will_use_blocks>12){
            block_idx = file_has_used_blocks - 1;
            all_blocks[block_idx] = file->fd_inode->i_sectors[block_idx];
            block_lba = block_bitmap_alloc(cur_part);
            if(block_lba==-1){
                printk("file_write: block_bitmap_alloc for situation 2 failed\n");
                return -1;
            }     
            ASSERT(file->fd_inode->i_sectors[12] == 0);
            indirect_block_table = file->fd_inode->i_sectors[12] = block_lba;
            block_idx = file_has_used_blocks;

            while(block_idx<file_will_use_blocks){
                block_lba = block_bitmap_alloc(cur_part);
                if(block_lba==-1){
                    printk("file_write: block_bitmap_alloc for situation 2 failed\n");
                    return -1;
                }
                if(block_idx<12){
                    ASSERT(file->fd_inode->i_sectors[block_idx] == 0);
                    file->fd_inode->i_sectors[block_idx] = all_blocks[block_idx] = block_lba;
                }
                else{
                    all_blocks[block_idx] = block_lba;
                }
                block_bitmap_idx = block_lba - cur_part->sb->data_start_lba;
                bitmap_sync(cur_part, block_bitmap_idx, BLOCK_BITMAP);
                block_idx++;
            }
            ide_write(cur_part->my_disk, indirect_block_table, all_blocks + 12, 1);
        }
        else {
            ASSERT(file->fd_inode->i_sectors[12] == 0);
            indirect_block_table = file->fd_inode->i_sectors[12];
             //printk("write2\n");
            ide_read(cur_part->my_disk, indirect_block_table, all_blocks + 12, 1);
            block_idx = file_has_used_blocks;
            while(block_idx<file_will_use_blocks){
                block_lba = block_bitmap_alloc(cur_part);
                if(block_lba==-1){
                    printk("file_write: block_bitmap_alloc for situation 3 failed\n");
                    return -1;
                }
                all_blocks[block_idx++] = block_lba;
                block_bitmap_idx = block_lba - cur_part->sb->data_start_lba;
                bitmap_sync(cur_part, block_bitmap_idx, BLOCK_BITMAP);//只是同步位图
            }
            ide_write(cur_part->my_disk, indirect_block_table, all_blocks + 12, 1);//一级间接块也是类似位图的，要一起改
        }
    }

    bool first_write_block = true;
    file->fd_pos = file->fd_inode->i_size - 1;

    while(bytes_written<count){
        memset(io_buf, 0, BLOCK_SIZE);
        sec_idx = file->fd_inode->i_size / BLOCK_SIZE;
        sec_lba = all_blocks[sec_idx];
        sec_off_bytes = file->fd_inode->i_size % BLOCK_SIZE;
        sec_left_bytes = BLOCK_SIZE - sec_off_bytes;

        chunk_size = size_left < sec_left_bytes ? size_left : sec_left_bytes;
        if(first_write_block){
             //printk("write3\n");
            ide_read(cur_part->my_disk, sec_lba, io_buf, 1);
            first_write_block = false;
        }
        memcpy(io_buf + sec_off_bytes, src, chunk_size);
        ide_write(cur_part->my_disk, sec_lba, io_buf, 1);
        printk("file write at lba 0x%x\n", sec_lba);
        src += chunk_size;
        file->fd_inode->i_size += chunk_size;
        file->fd_pos += chunk_size;
        bytes_written += chunk_size;
        size_left -= chunk_size;
    }
    inode_sync(cur_part, file->fd_inode, io_buf);
    sys_free(all_blocks);
    sys_free(io_buf);
    return bytes_written;
}

int32_t file_close(struct file* file){
    if(file==NULL){
        return -1;
    }
    file->fd_inode->write_deny = false;
    inode_close(file->fd_inode);
    file->fd_inode = NULL;
    return 0;
}

int32_t file_read(struct file* file,void* buf,uint32_t count){//从文件中读取count个字节到file
    uint8_t *buf_dst = (uint8_t *)buf;
    uint32_t size = count, size_left = size;
    if((file->fd_pos+count)>file->fd_inode->i_size){
        size = file->fd_inode->i_size - file->fd_pos;
        size_left = size;
        if(size==0){
            //printk("file _read size==0\n");
            return -1;
        }
    }
    printf("file read 0\n");
         see_uporg();
    printf("file read 1\n");

    uint8_t *io_buf = sys_malloc(BLOCK_SIZE);
    printf("file read 1 *io_buf: 0x%x\n",io_buf);
    if(io_buf==NULL){
        printk("file_read: sys_malloc for io_buf failed\n");
        return -1;
    }
    struct arena* a = block2arena((struct mem_block*) io_buf);
    printf("blocks_per_arena malloc  wai: %d  a->cnt : %d \n",a->desc->blocks_per_arena,a->cnt);
    (uint8_t *)io_buf;
    uint32_t *all_blocks = (uint32_t *)sys_malloc(BLOCK_SIZE + 48);
    if(all_blocks==NULL){
        printk("file_read: sys_malloc for all_blocks failed\n");
        return -1;
    }

     
    uint32_t block_read_start_idx = file->fd_pos / BLOCK_SIZE;
    uint32_t block_read_end_idx = (file->fd_pos + size) / BLOCK_SIZE;

    uint32_t read_blocks = block_read_start_idx - block_read_end_idx;

    ASSERT(block_read_start_idx < 139 && block_read_end_idx < 139);

    int indirect_block_table;
    uint32_t block_idx;

   
    if(read_blocks==0){
        ASSERT(block_read_start_idx == block_read_end_idx);
        if(block_read_end_idx<12){
            block_idx = block_read_end_idx;
            all_blocks[block_idx] = file->fd_inode->i_sectors[block_idx];
        }
        else{
            indirect_block_table = file->fd_inode->i_sectors[12];
            ide_read(cur_part->my_disk, indirect_block_table, all_blocks + 12, 1);
        }
    }
    else{
        if(block_read_end_idx<12){
            block_idx = block_read_start_idx;
            while(block_idx<=block_read_end_idx){
                all_blocks[block_idx] = file->fd_inode->i_sectors[block_idx];
                block_idx++;
            }
        }
        else if(block_read_start_idx<12&&block_read_end_idx>=12){
            block_idx = block_read_start_idx;
            while(block_idx<12){
                all_blocks[block_idx] = file->fd_inode->i_sectors[block_idx];
                block_idx++;
            }
            ASSERT(file->fd_inode->i_sectors[12] != 0);

            indirect_block_table = file->fd_inode->i_sectors[12];
            ide_read(cur_part->my_disk, indirect_block_table, all_blocks + 12, 1);
        }
        else{
            ASSERT(file->fd_inode->i_sectors[12] != 0);

            indirect_block_table = file->fd_inode->i_sectors[12];
            ide_read(cur_part->my_disk, indirect_block_table, all_blocks + 12, 1);
        }
    }
    printf("file read 2\n");
    uint32_t sec_idx, sec_lba, sec_off_bytes, sec_left_bytes, chunk_size;
    uint32_t bytes_read = 0;
    printf("read size: %d \n",size);
    see_uporg();
    while(bytes_read<size){
        sec_idx = file->fd_pos / BLOCK_SIZE;
        sec_lba = all_blocks[sec_idx];
        sec_off_bytes = file->fd_pos % BLOCK_SIZE;
        sec_left_bytes = BLOCK_SIZE - sec_off_bytes;
        chunk_size = size_left < sec_left_bytes ? size_left : sec_left_bytes;

        memset(io_buf, 0, BLOCK_SIZE);

// struct arena{
//     struct mem_block_desc *desc;
//     uint32_t cnt;
//     bool large;
// };

        printf("blocks_per_arena malloc   wai ff: %d  a->cnt : %d  block_size %d  desc %x\n",\
        a->desc->blocks_per_arena,a->cnt,a->desc->block_size,a->desc);
       // int temp_desc=a->desc;int temp_cnt=a->cnt;bool temp_size=a->large;
        void *temp=sys_malloc(4084);
        struct arena* temp1=temp-0xc;
        printf("temp :%x  a: %x",temp,a);
       memcpy(temp1,a,PG_SIZE);
        ide_read(cur_part->my_disk, sec_lba, io_buf, 1);
   //            printf("blocks_per_arena malloc   wai bb: %d  a->cnt : %d  block_size %d  desc %x\n",\
     //   a->desc->blocks_per_arena,a->cnt,a->desc->block_size,a->desc);
        memcpy(a,temp1,PG_SIZE);
        see_uporg();
        
        temp1->desc = NULL;
	    temp1->cnt =1;
	    temp1->large = true;
        sys_free(temp);
        printf("1111111\n");
       // a->desc=temp_desc;a->cnt=temp_cnt,a->large=temp_size;
        see_uporg();
       printf("blocks_per_arena malloc   wai bb: %d  a->cnt : %d  block_size %d  desc %x\n",\
        a->desc->blocks_per_arena,a->cnt,a->desc->block_size,a->desc);
        memcpy(buf_dst, io_buf + sec_off_bytes, chunk_size);

        buf_dst += chunk_size;
        file->fd_pos += chunk_size;
        bytes_read += chunk_size;
        size_left -= chunk_size;
    }
    
    printf("all block s :%d   ",sizeof(*all_blocks));
    see_uporg();
    sys_free(all_blocks);

    printf("all block s :%d   ",io_buf);
    //see_uporg();
    sys_free(io_buf);
    //printf("blocks_per_arena malloc  wai: %d  a->cnt : %d \n",a->desc->blocks_per_arena,a->cnt);

    printk("bytes_read:%d\n",bytes_read);
    return bytes_read;
}