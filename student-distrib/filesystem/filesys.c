/* file.h - Defines for file operations 
*/

#include "filesys.h"
#include "../lib.h"
#include "../devices/rtc.h"

#define FAILURE     -1
#define SUCCESS     0

boot_blk_t* fs_start_ptr = NULL;
// uint32_t file_read_pos;
// uint32_t dir_read_pos;

file_ops_table_t operation_set[FILE_TYPE_NUM] = {
    {rtc_open_intf, rtc_close_intf, rtc_write_intf, rtc_read_intf},
    {dir_open_intf, dir_close, dir_write_intf, dir_read_intf},
    {file_open_intf, file_close, file_write_intf, file_read_intf}
};

/*
 * get_file_ops
 *   DESCRIPTION: get file operation based on file type
 *   INPUTS: file type
 *   OUTPUTS: None
 *   RETURN VALUE: file op table
 */
file_ops_table_t get_file_ops(uint32_t type){
    return operation_set[type];
}

/*
 * init_filesys
 *   DESCRIPTION: initialize file system
 *   INPUTS: pointer to the start of filesys
 *   OUTPUTS: None
 *   RETURN VALUE: None
 */
void init_filesys(uint32_t* filesys_addr) {
    fs_start_ptr = (boot_blk_t*) filesys_addr;
}

/*
 * dir_open
 *   DESCRIPTION: open the file with file name
 *   INPUTS: inode_ptr
 *           fname - name of the dir to be opened
 *   OUTPUTS: inode in file descriptor will be set to current file's inode
 *   RETURN VALUE: 0 - SUCCESS: means the dentry is successfully found
 *                 -1 - FAILURE: means the dentry is not found
 */
int32_t dir_open(int32_t* inode_ptr, const char* fname) {
    /* if the ptr's are invalid */
    if (fs_start_ptr == NULL || fname == NULL) return FAILURE;
    dentry_t dir_dentry;
    if (read_dentry_by_name(fname, &dir_dentry) == FAILURE) return FAILURE;
    *inode_ptr = dir_dentry.inode;
    // dir_read_pos = 0;
    return SUCCESS;
}

/*
 * dir_open_intf
 *   DESCRIPTION: general wrapper function for uniform interface
 *   INPUTS: filename
 *   OUTPUTS: None
 *   RETURN VALUE: inode number for success and -1 for failure
 */
int32_t dir_open_intf(const uint8_t* fname){
    int32_t inode;
    int32_t ret;
    ret = dir_open(&inode, (char*)fname);
    if (ret==SUCCESS)
        return inode;
    return FAILURE;
}

/*
 * dir_close
 *   DESCRIPTION: close the file specified by the inode
 *   INPUTS: not used since inode is not for dir
 *   OUTPUTS: none
 *   RETURN VALUE: 0 - SUCCESS: means the dentry is successfully found
 *                 -1 - FAILURE: means the dentry is not found
 */
int32_t dir_close(int32_t* inode_ptr) {
    if (fs_start_ptr == NULL) return FAILURE;
    return SUCCESS;
}

/*
 * dir_write
 *   DESCRIPTION: write to the dir (not supported in Checkpoint 2)
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: 0 - SUCCESS: means the dentry is successfully found
 *                 -1 - FAILURE: means the dentry is not found
 */
int32_t dir_write() {
    return FAILURE;
}

/*
 * dir_write_intf
 *   DESCRIPTION: general wrapper function for uniform interface
 *   INPUTS: USELESS
 *   OUTPUTS: None
 *   RETURN VALUE: follow the function inside wrapper
 */
int32_t dir_write_intf(int32_t inode, const void* buf, int32_t nbytes){
    return dir_write();
}

/*
 * dir_read
 *   DESCRIPTION: 
 *   INPUTS: inode - an index of inode block
 *           buf - the data read will be written into buf
 *           length - the number of bytes to be read
 *   OUTPUTS: the buffer is filled with data read from the file
 *   RETURN VALUE: number of bytes read from the file
 */
int32_t dir_read(int32_t inode, uint32_t* offset, char* buf, int32_t length) {
    /* check if the file system is loaded, if the buf is NULL, if all files are read already */
    if (fs_start_ptr == NULL || buf == NULL || *offset >= MAX_FILE_NUM) return FAILURE;
    int32_t len_to_read = length;
    /* find current file's dentry info */
    dentry_t den;
    int32_t ret;
    if (fs_start_ptr == NULL || &den == NULL) return FAILURE;
    ret = read_dentry_by_index(*offset, &den);
    // ret=FAILURE means we are reaching the end, so return SUCCESS
    // this might be weird, but we do this for the "ls" program
    // when all the files have been read, we should return SUCCESS
    if (ret == FAILURE) return SUCCESS;
    /* determine number of bytes to read */
    if (length > MAX_FILENAME_LEN) {
        len_to_read = MAX_FILENAME_LEN;
    }
    if (len_to_read > strlen(den.file_name)) {
        len_to_read = strlen(den.file_name);
    }
    /* copy data from dentry info to buffer */
    memcpy(buf, den.file_name, len_to_read);
    /* increase index of files read */
    *offset += 1;
    return len_to_read;
}

/*
 * dir_read_intf
 *   DESCRIPTION: general wrapper functino for uniform interface
 *   INPUTS: inode - index to inode, not inode block
 *           offset - pointer to file_pos in fda
 *           buf
 *           nbytes
 *   OUTPUTS: None
 *   RETURN VALUE: follow the function inside wrapper
 */
int32_t dir_read_intf(int32_t inode, uint32_t* offset, void* buf, int32_t nbytes){
    return dir_read(inode, offset, (char*)buf, nbytes);
}

/*
 * file_open
 *   DESCRIPTION: open the file with file name
 *   INPUTS: inode_ptr
 *           fname - file name of the file to be opened
 *   OUTPUTS: inode in file descriptor will be set to current file's inode
 *   RETURN VALUE: 0 - SUCCESS: means the dentry is successfully found
 *                 -1 - FAILURE: means the dentry is not found
 */
int32_t file_open(int32_t* inode_ptr, const char* fname) {
    /* if the ptr's are invalid */
    if (fs_start_ptr == NULL || fname == NULL) return FAILURE;
    dentry_t file_dentry;
    if (read_dentry_by_name(fname, &file_dentry) == FAILURE) return FAILURE;
    // printf("initial file system %x \n", fs_start_ptr);
    printf("open file \n");
    printf("file name %s \n", (&file_dentry)->file_name);
    *inode_ptr = file_dentry.inode;
    // file_read_pos = 0;
    return SUCCESS;
}

/*
 * file_open_intf
 *   DESCRIPTION: general wrapper function for uniform interface
 *   INPUTS: fname
 *   OUTPUTS: None
 *   RETURN VALUE: inode number for success and -1 for failure
 */
int32_t file_open_intf(const uint8_t* fname){
    int32_t inode, ret;
    ret = file_open(&inode, (char*)fname);
    if (ret==SUCCESS)
        return inode;
    return FAILURE;
}

/*
 * file_close
 *   DESCRIPTION: close the file specified by the inode
 *   INPUTS: inode number pointer in file descriptor
 *   OUTPUTS: inode in file descriptor will change to invalid
 *   RETURN VALUE: 0 - SUCCESS: means the dentry is successfully found
 *                 -1 - FAILURE: means the dentry is not found
 */
int32_t file_close(int32_t* inode_ptr) {
    // printf("close file \n");
    if (fs_start_ptr == NULL) return FAILURE;
    /* assign an invaid inode */
    *inode_ptr = -1;
    return SUCCESS;
}

/*
 * file_write
 *   DESCRIPTION: write to the file (not supported in Checkpoint 2)
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: 0 - SUCCESS: means the dentry is successfully found
 *                 -1 - FAILURE: means the dentry is not found
 */
int32_t file_write() {
    return FAILURE;
}

/*
 * file_write_intf
 *   DESCRIPTION: general wrapper function for uniform interface
 *   INPUTS: USELESS
 *   OUTPUTS: None
 *   RETURN VALUE: -1
 */
int32_t file_write_intf(int32_t inode, const void* buf, int32_t nbytes){
    return FAILURE;
}

/*
 * file_read
 *   DESCRIPTION: 
 *   INPUTS: inode - an index to inode block
 *           buf - the data read will be written into buf
 *           length - the number of bytes to be read
 *   OUTPUTS: the buffer is filled with data read from the file
 *   RETURN VALUE: number of bytes read from the file
 */
int32_t file_read(int32_t inode, uint32_t* offset, char* buf, int32_t length) {
    if (fs_start_ptr == NULL) return FAILURE;
    // printf("read file \n");
    int32_t nbytes;
    nbytes = 0;
    // printf("offset in file read %d \n", *offset);
    /* read until all bytes in the file are written into the buffer */
    nbytes = read_data(inode, *offset, (char*) buf, length);
    if (nbytes == FAILURE) return FAILURE;
    // printf("file read nbytes %d \n", nbytes);
    *offset += nbytes;
    // printf("offset in file read %d \n", *offset);
    return nbytes;
}

/*
 * file_read_intf 
 *   DESCRIPTION: general wrapper function for uniform interface
 *   INPUTS: inode - inode block index, not inode block itself
 *           offset - pointer to file_pos in fda
 *           buf
 *           nbytes
 *   OUTPUTS: None
 *   RETURN VALUE: follow the function inside wrapper
 */
int32_t file_read_intf(int32_t inode, uint32_t* offset, void* buf, int32_t nbytes){
    return file_read(inode, offset, (char*)buf, nbytes);
}


/*
 * read_dentry_by_name
 *   DESCRIPTION: 
 *   INPUTS: fname - the name of file to be opened
 *           dentry - the dentry struct to be output to
 *   OUTPUTS: dentry struct is filled with info of specified file
 *   RETURN VALUE: 0 - SUCCESS: means the dentry is successfully found
 *                 -1 - FAILURE: means the dentry is not found
 */
int32_t read_dentry_by_name(const char* fname, dentry_t* dentry) {
    /* if the file system is not loaded OR if the file dentry ptr is invalid, return -1 */
    if (fs_start_ptr == NULL || dentry == NULL) return FAILURE;
    /* if the length of the file name is larger than 32B, truncate it */
    int32_t name_len = strlen(fname);
    // printf("The length of the file name %d \n", name_len);
    if(name_len == 0) return FAILURE;

    if(strlen((int8_t*) fname) > MAX_FILENAME_LEN) return FAILURE;
    int32_t i;
    char input_name[MAX_FILENAME_LEN+1];
    if (name_len > MAX_FILENAME_LEN) {
        memcpy(input_name, fname, MAX_FILENAME_LEN);
        input_name[MAX_FILENAME_LEN] = (char) '\0';
        name_len = MAX_FILENAME_LEN;
    }
    else{
        memcpy(input_name, fname, name_len);
        input_name[name_len]='\0';
    }
    
    /* char array to save file names in dentry and append \0 */
    char temp_name[33];
    /* loop over files to find the queried file */
// fs_start_ptr->num_dir_entries?
    for(i = 0; i < MAX_FILE_NUM; i++) {
        dentry_t* file = &(fs_start_ptr -> dir_entries[i]);
        /* append \0 to file name */
        memcpy(temp_name, file -> file_name, MAX_FILENAME_LEN);
        temp_name[32] = (char) '\0';
        if (0 == strncmp(input_name, temp_name, MAX_FILENAME_LEN)) {
            *dentry = *file;
            return SUCCESS;
        }
    }
    return FAILURE;
}


/*
 * read_dentry_by_index
 *   DESCRIPTION: 
 *   INPUTS: index - dentry index, range from 0 to MAX_FILE_NUM - 1
 *           dentry - the dentry struct to be output to
 *   OUTPUTS: dentry struct is filled with info of specified file
 *   RETURN VALUE: 0 - SUCCESS: means the dentry is successfully found
 *                 -1 - FAILURE: means the dentry is not found
 */
int32_t read_dentry_by_index(uint32_t index, dentry_t* dentry) {
    /* if index is out of range, return -1 */
    if (index >= fs_start_ptr->num_dir_entries) return FAILURE;

    *dentry = fs_start_ptr->dir_entries[index];
    return SUCCESS;
}


/*
 * read_data
 *   DESCRIPTION: read specified length of data into buffer from the given inode
 *   INPUTS: inode - index of the inode to read from
 *           offset - the starting reading point in the inode
 *           buf - the data read will be written into buf
 *           length - the number of bytes to be read
 *   OUTPUTS: buf will be filled with data from inode
 *   RETURN VALUE: number of bytes copied into buffer or FAILURE(-1) if copy is failed
 */
int32_t read_data(uint32_t inode, uint32_t offset, char* buf, uint32_t length) {
    /* check validality of inputs */
    /* if the file system is not loaded or buf ptr is not defined or inode is out of range, return -1 */
    if (fs_start_ptr == NULL || buf == NULL || inode >= fs_start_ptr -> num_inodes) return FAILURE;
    // printf("fail1");
    inode_blk_t* inode_blk = (inode_blk_t*) fs_start_ptr + 1 + inode;     // inode is 0 started
    // printf("The size of inode is %d \n", inode_blk->size);
    /* check if the index and offset are valid */
    if (offset >= inode_blk->size) return SUCCESS;
    // printf("offset in read_data %d \n", offset);
    // printf("fail2");
    /* check if all data can be obtained */
    uint32_t len_to_get;
    if (offset + length <= inode_blk->size) {
        len_to_get = length;
    } else {
        len_to_get = inode_blk->size - offset;
    }
    /* figure out which blocks to read from */
    uint32_t first_blk_to_read, last_blk_to_read, curr_blk_idx, nbytes, begin, end, i;
    data_blk_t* curr_blk;
    first_blk_to_read = offset / FS_BLK_SIZE;
    // printf("The first blk is %d \n", first_blk_to_read);   
    last_blk_to_read = (offset + len_to_get) / FS_BLK_SIZE;
    // printf("The last blk is %d \n", last_blk_to_read); 
    nbytes = 0;
    /* loop over all data block to read */
    for (i = first_blk_to_read; i <= last_blk_to_read; i++) {
        curr_blk_idx = inode_blk->data[i];
        curr_blk = (data_blk_t*) fs_start_ptr + fs_start_ptr -> num_inodes + curr_blk_idx + 1;         // curr_blk_idx is 0 started
        /* determine the start position at current block */
        if (i*FS_BLK_SIZE < offset) {
            begin = offset - i*FS_BLK_SIZE;
        } else {
            begin = 0;
        }
        /* determine the end position at current block */
        if ((i+1)*FS_BLK_SIZE > offset + len_to_get) {
            end = offset + len_to_get - i*FS_BLK_SIZE;
        } else {
            end = FS_BLK_SIZE;
        }
        /* copy end - begin bytes to the buffer */
        // printf("fail3");
        memcpy((char*) buf + nbytes, (char*) curr_blk + begin, end-begin);
        // printf("fail4");
        nbytes += end - begin;
    }
    // printf("read data %d \n", nbytes);
    return nbytes;
}



