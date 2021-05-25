#ifndef _FS_H
#define _FS_H

#include "../types.h"

#define MAX_FILENAME_LEN    32
#define MAX_FILE_NUM        63
#define FS_BLK_SIZE         4096
#define FS_BLK_SIZE_4B      1024

#define FILE_TYPE_RTC       0
#define FILE_TYPE_DIR       1
#define FILE_TYPE_REGULAR   2
#define FILE_TYPE_NUM       3

typedef struct {
    char file_name[MAX_FILENAME_LEN];
    uint32_t file_type;
    uint32_t inode;
    uint8_t reserved[24];
} dentry_t;

typedef struct {
    uint32_t num_dir_entries;
    uint32_t num_inodes;
    uint32_t num_data_blocks;
    uint8_t reserved[52];
    dentry_t dir_entries[MAX_FILE_NUM];
} boot_blk_t;

typedef struct {
    uint32_t size;
    uint32_t data[FS_BLK_SIZE_4B - 1];
} inode_blk_t;

typedef struct {
    uint32_t data[FS_BLK_SIZE_4B];
} data_blk_t;

void init_filesys(uint32_t* filesys_addr);

// close operation doesn't need interface
// to guarantee uniform interface to system call
int32_t dir_open(int32_t* inode_ptr, const char* fname);
int32_t dir_open_intf(const uint8_t* fname);

int32_t dir_close(int32_t* inode_ptr);

int32_t dir_write();
int32_t dir_write_intf(int32_t inode, const void* buf, int32_t nbytes);

int32_t dir_read(int32_t inode, uint32_t* offset, char* buf, int32_t length);
int32_t dir_read_intf(int32_t inode, uint32_t* offset, void* buf, int32_t nbytes);

int32_t file_open(int32_t* inode_ptr, const char* fname);
int32_t file_open_intf(const uint8_t* fname);

int32_t file_close(int32_t* inode_ptr);

int32_t file_write();
int32_t file_write_intf(int32_t inode, const void* buf, int32_t nbytes);

int32_t file_read(int32_t inode, uint32_t* offset, char* buf, int32_t length);
int32_t file_read_intf(int32_t inode, uint32_t* offset, void* buf, int32_t nbytes);


int32_t read_dentry_by_name(const char* fname, dentry_t* dentry);
int32_t read_dentry_by_index(uint32_t index, dentry_t* dentry);
int32_t read_data(uint32_t inode, uint32_t offset, char* buf, uint32_t length);

/* define global variables */
extern boot_blk_t* fs_start_ptr;
// extern dentry_t file_dentry;
// extern dentry_t dir_dentry;
// extern uint32_t file_read_pos;
// extern uint32_t dir_read_pos;

// general file operation abstraction
// function interface for all devices
typedef struct{
    int32_t (*open)(const uint8_t* filename);
    int32_t (*close)(int32_t* inode_ptr);
    int32_t (*write)(int32_t inode, const void* buf, int32_t nbytes);
    int32_t (*read)(int32_t inode, uint32_t* offset, void* buf, int32_t nbytes);
}file_ops_table_t;

#define FILE_FLAG_IN_USE    1
#define FILE_FLAG_FREE      0

typedef struct {
    file_ops_table_t file_op;
    int32_t inode;
    uint32_t file_pos;
    int32_t flags;
} file_desc_entry_t;

extern file_ops_table_t operation_set[FILE_TYPE_NUM];

file_ops_table_t get_file_ops(uint32_t type);

#endif /* _FS_H */
