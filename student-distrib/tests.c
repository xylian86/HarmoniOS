#include "tests.h"
#include "x86_desc.h"
#include "lib.h"
#include "devices/rtc.h"
#include "devices/keyboard.h"
#include "devices/i8259.h"
#include "filesystem/filesys.h"
#include "terminal.h"
#include "do_syscall.h"
#include "types.h"
#include "process_crtl.h"

#define PASS 1
#define FAIL 0

#define FAILURE -1
#define SUCCESS 0

/* format these macros as you see fit */
#define TEST_HEADER 	\
	printf("[TEST %s] Running %s at %s:%d\n", __FUNCTION__, __FUNCTION__, __FILE__, __LINE__)
#define TEST_OUTPUT(name, result)	\
	printf("[TEST %s] Result = %s\n", name, (result) ? "PASS" : "FAIL");

static inline void assertion_failure(){
	/* Use exception #15 for assertions, otherwise
	   reserved by Intel */
	asm volatile("int $15");
}

// open system call wrapper
extern int32_t __ece391_read (int32_t fd, void* buf, int32_t nbytes);
extern int32_t __ece391_open (const uint8_t* filename);

#define DO_CALL(name,number)       \
asm volatile ("                    \
.GLOBL " #name "                  ;\
" #name ":                        ;\
        PUSHL	%EBX              ;\
	MOVL	$" #number ",%EAX ;\
	MOVL	4(%ESP),%EBX      ;\
	MOVL	8(%ESP),%ECX     ;\
	MOVL	12(%ESP),%EDX     ;\
	INT	$0x80             ;\
1:	POPL	%EBX              ;\
")




/* Checkpoint 1 tests */

/* IDT Test - Example
 * 
 * Asserts that first 10 IDT entries are not NULL
 * Inputs: None
 * Outputs: PASS/FAIL
 * Side Effects: None
 * Coverage: Load IDT, IDT definition
 * Files: x86_desc.h/S
 */
int idt_test(){
	TEST_HEADER;

	int i;
	int result = PASS;
	for (i = 0; i < 10; ++i){
		if ((idt[i].offset_15_00 == NULL) && 
			(idt[i].offset_31_16 == NULL)){
			assertion_failure();
			result = FAIL;
		}
	}

	return result;
}

// add more tests here

/* divide by error Test - Example
 * 
 * Asserts that first 10 IDT entries are not NULL
 * Inputs: None
 * Outputs: error message/none
 * Side Effects: if we divide the vale by 0, we will meet the fault
 * Coverage: Load IDT, IDT definition
 * Files: x86_desc.h/S
 */
void div_by_error_test() {
	TEST_HEADER;
	int a = 0;
	int b;
	b = 1/a;
}

/* common_exception_tests
 * 
 * This function can test all exceptions based on INT $vec in ASM. 
 * Inputs: vector VEC
 * Outputs: NONE
 * Side Effects: print a message if we set exception using INT instruction
 * Coverage: Load IDT, IDT definition
 * Files: x86_desc.h/S
 */
void common_exception_tests(unsigned int VEC)
{
	//condition check
	switch (VEC)
	{
	case 0x00:					//represetn IDT[0],etc.
		asm("int $0x00");		//For the following code, they have same functions.
		break;
	case 0x01:				
		/* code */
		asm("int $0x01");
		break;
	case 0x02:
		/* code */
		asm("int $0x02");
		break;
	case 0x03:
		asm("int $0x03");
		/* code */
		break;
	case 0x04:
		asm("int $0x04");
		/* code */
		break;
	case 0x05:
		asm("int $0x05");
		/* code */
		break;
	case 0x06:
		asm("int $0x06");
		/* code */
		break;
	case 0x07:
		asm("int $0x07");
		/* code */
		break;
	case 0x08:
		asm("int $0x08");
		/* code */
		break;
	case 0x09:
		asm("int $0x09");
		/* code */
		break;
	case 0x0A:
		asm("int $0x0A");
		/* code */
		break;
	case 0x0B:
		asm("int $0x0B");
		/* code */
		break;
	case 0x0C:
		asm("int $0x0C");
		/* code */
		break;
	case 0x0D:
		asm("int $0x0D");
		/* code */
		break;
	case 0x0E:
		asm("int $0x0E");
		/* code */
		break;
	case 0x0F:
		asm("int $0x0F");
		/* code */
		break;
	case 0x10:
		asm("int $0x10");
		/* code */
		break;
	case 0x11:
		asm("int $0x11");
		/* code */
		break;
	case 0x12:
		asm("int $0x12");
		/* code */
		break;
	case 0x13:
		asm("int $0x13");
		/* code */
		break;
	default:
		break;
	}
	return;
}

void selective_excp1(){
	asm("int $0x13");
}

void selective_excp2(){
	asm("int $0x0F");
}

void selective_excp3(){
	asm("int $0x07");
}

/* PIC test enable invalid irq
 * 
 * Inputs: None
 * Outputs: None
 * Side Effects: no irq is unmasked, an error message printed on screen
 * Coverage: invalid input to the enable_irq function
 * Files: i8259.c
 */
void pic_enable_invalid_test(){
    TEST_HEADER;
    printf("%x    ", master_mask);
    printf("%x \n", slave_mask);
	/* 16 and 256 are both invalid irq's */
	enable_irq(16);
	enable_irq(256);
    printf("%x    ", master_mask);
    printf("%x \n", slave_mask);
}

/* PIC test enable valid irq
 * 
 * Inputs: None
 * Outputs: None
 * Side Effects: irq 0 and 8 are unmasked
 * Coverage: valid input to the enable_irq function
 * Files: i8259.c
 */
void pic_enable_valid_test(){
    TEST_HEADER;
    printf("%x    ", master_mask);
    printf("%x \n", slave_mask);
	/* 16 and 256 are both invalid irq's */
	enable_irq(IRQ_RTC);
	enable_irq(KEYBOARD_NUMBER);
    printf("%x    ", master_mask);
    printf("%x \n", slave_mask);
}

/* PIC test disable invalid irq
 * 
 * Inputs: None
 * Outputs: None
 * Side Effects: no irq is masked again, an error message printed on screen
 * Coverage: invalid input to the disable_irq function
 * Files: i8259.c
 */
void pic_disable_invalid_test(){
    TEST_HEADER;
    printf("%x    ", master_mask);
    printf("%x \n", slave_mask);
	/* 16 and 256 are both invalid irq's */
	disable_irq(16);
	disable_irq(256);
    printf("%x    ", master_mask);
    printf("%x \n", slave_mask);
}

/* PIC test disable valid irq
 * 
 * Inputs: None
 * Outputs: None
 * Side Effects: irq 0 and 8 are masked
 * Coverage: valid input to the disable_irq function
 * Files: i8259.c
 */
void pic_disable_valid_test(){
    TEST_HEADER;
    printf("%x    ", master_mask);
    printf("%x \n", slave_mask);
	/* 16 and 256 are both invalid irq's */
	disable_irq(IRQ_RTC);
	disable_irq(KEYBOARD_NUMBER);
    printf("%x    ", master_mask);
    printf("%x \n", slave_mask);
}

/*
 * paging_test1
 *   DESCRIPTION: deref test case
 *   INPUTS: none 
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: raise one page fault for second pointer which is invalid
 */
void paging_test1(){
	TEST_HEADER;
	uint32_t* ptr;
	uint32_t val = 2;
	ptr = &val;
	// defef the val
	printf("valid deref(ptr) -> %d\n", *ptr);
	// def some null
	printf("invalid deref(NULL)\n");
	ptr = NULL;
	val = *ptr;
}

/*
 * paging_test2
 *   DESCRIPTION: deref test case
 *   INPUTS: none 
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: raise one page fault for second pointer which is invalid
 */
void paging_test2(){
	TEST_HEADER;
	uint32_t val;
	printf("valid deref(0x00400000)\n");
	val = *((char *) (0x00400000));
	printf("invalid deref(0xB5000)\n");
	val = *((char *) 0xB5000);
}

/*
 * paging_test3
 *   DESCRIPTION: deref test case
 *   INPUTS: none 
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: raise one page fault for second pointer which is invalid
 */
void paging_test3(){
	TEST_HEADER;
	uint32_t val;
	printf("valid deref(0xB8102)\n");
	val = *((char *) 0xB8102);
	printf("invalid deref(0xB9000)\n");
	val = *((char *) 0xB9000);
}

/*
 * paging_test4
 *   DESCRIPTION: deref test case
 *   INPUTS: none 
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: raise one page fault for second pointer which is invalid
 */
void paging_test4(){
	TEST_HEADER;
	uint32_t val;
	printf("valid deref(0x007FFFFF)\n");
	val =  *((char *) 0x007FFFFF);
	printf("invalid deref(0x10700000)\n");
	val =  *((char *) 0x10700000);
}

/*
 * paging_test5
 *   DESCRIPTION: deref test case
 *   INPUTS: none 
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: all valid tests 
 */
void paging_test5(){
	TEST_HEADER;
	uint32_t val;
	uint32_t tmp_array[3] = {5,6,7};
	printf("valid deref(0x004FF132)\n");
	val =  *((char *) 0x004FF132);
	printf("valid deref(0x000B8999)\n");
	val =  *((char *) 0x000B8999);
	printf("valid deref(array)\n");
	val =  *(tmp_array);
	printf("valid deref(array+1)\n");
	val =  *(tmp_array+1);
	printf("valid deref(array+2)\n");
	val =  *(tmp_array+2);
}

/*
 * continue_testing
 *   DESCRIPTION: pop out some function from array and execute it
 *   INPUTS: none 
 *   OUTPUTS: none
 *   RETURN VALUE: none 
 *   SIDE EFFECTS: pop out from data structure like stack
 */
void continue_testing(){
	fun_ptr fp= pop_back_func();
	if ((void *)fp != NULL)
		fp();
}

/* Checkpoint 2 tests */

/*
 * read_dentry_by_valid_name_test
 *   DESCRIPTION: test read_dentry_by_name with an existing file frame0.txt
 *   INPUTS: none 
 *   OUTPUTS: none
 *   RETURN VALUE: PASS/FAIL
 */
int read_dentry_by_valid_name_test(){
    clear();
    TEST_HEADER;
    int32_t ret;
    dentry_t dentry;
    ret = read_dentry_by_name("frame0.txt", &dentry);
    if (ret == FAILURE) return FAIL;
    return PASS;
}

/*
 * read_dentry_by_invalid_name_test
 *   DESCRIPTION: test read_dentry_by_name with an nonexisting file fake.txt
 *   INPUTS: none 
 *   OUTPUTS: none
 *   RETURN VALUE: PASS/FAIL
 */
int read_dentry_by_invalid_name_test(){
    TEST_HEADER;
    int32_t ret;
    dentry_t dentry;
    ret = read_dentry_by_name("fake.txt", &dentry);
    if (ret == SUCCESS) return FAIL;
    return PASS;
}

/*
 * read_dentry_by_dir_name_test
 *   DESCRIPTION: test read_dentry_by_name with the dir name "."
 *   INPUTS: none 
 *   OUTPUTS: none
 *   RETURN VALUE: PASS/FAIL
 */
int read_dentry_by_dir_name_test(){
    TEST_HEADER;
    int32_t ret;
    dentry_t dentry;
    ret = read_dentry_by_name(".", &dentry);
    if (ret == FAILURE) return FAIL;
    return PASS;
}

/*
 * read_dentry_by_valid_index_test
 *   DESCRIPTION: read_dentry_by_index with an valid inode index = 0
 *   INPUTS: none 
 *   OUTPUTS: none
 *   RETURN VALUE: PASS/FAIL
 */
int read_dentry_by_valid_index_test(){
    // clear();
    TEST_HEADER;
    int32_t ret;
    dentry_t dentry;
    uint32_t index = 0;
    ret = read_dentry_by_index(index, &dentry);
    /* if not regular file type */
    if (dentry.file_type != 1) return FAIL;
    if (ret == FAILURE) return FAIL;
    return PASS;
}

/*
 * read_dentry_by_invalid_index_test
 *   DESCRIPTION: read_dentry_by_index with an invalid inode index = -1
 *   INPUTS: none 
 *   OUTPUTS: none
 *   RETURN VALUE: PASS/FAIL
 */
int read_dentry_by_invalid_index_test(){
    TEST_HEADER;
    int32_t ret;
    dentry_t dentry;
    uint32_t index = -1;
    ret = read_dentry_by_index(index, &dentry);
    if (ret == SUCCESS) return FAIL;
    return PASS;
}



/*
 * file_read_test1
 *   DESCRIPTION: test file_read with small file - frame0.txt
 *   INPUTS: none 
 *   OUTPUTS: The file data read from "frame0.txt"
 *   RETURN VALUE: PASS/FAIL
 */
int file_read_test1(){
    clear();
    TEST_HEADER;
    /* file descripter, reserved for cp3 */
    int32_t fd;
    /* a buffer to store the data */
    // char buf[256];
    char buf[4096];
    int32_t size, i;
    uint32_t offset;
    file_open(&fd, "frame0.txt");
    // file_open(&fd, "fs_test.txt");
    offset = 0;
    printf("inode %d \n", fd);
    // size = file_read(fd, &offset, buf, 256);
    size = file_read(fd, &offset, buf, 4096);
	printf("offset %d \n", offset);
    printf("Bytes read: %d \n", size);
    for (i = 0; i < size; i++) {
        putc(buf[i]);
    }
    file_close(&fd);
    return PASS;
}

/*
 * file_read_test2
 *   DESCRIPTION: test file_read with small file - frame1.txt
 *   INPUTS: none 
 *   OUTPUTS: The file data read from "frame1.txt"
 *   RETURN VALUE: PASS/FAIL
 */
int file_read_test2(){
    clear();
    TEST_HEADER;
    /* file descripter, reserved for cp3 */
    int32_t fd;
    /* a buffer to store the data */
    char buf[256];
    int32_t size, i;
    uint32_t offset;
    file_open(&fd, "frame1.txt");
    offset = 0;
    printf("inode %d \n", fd);
    size = file_read(fd, &offset, buf, 256);
    printf("Bytes read: %d \n", size);
    for (i = 0; i < size; i++) {
        putc(buf[i]);
    }
    file_close(&fd);
    return PASS;
}

/*
 * file_read_test3
 *   DESCRIPTION: test executable file - grep and show the head
 *   INPUTS: none 
 *   OUTPUTS: The file data read from "grep"
 *   RETURN VALUE: PASS/FAIL
 */
int file_read_test3(){
    clear();
    TEST_HEADER;
    /* file descripter, reserved for cp3 */
    int32_t fd;
    /* a buffer to store the data */
    char buf[10];
    int32_t size, i;
    uint32_t offset;
    file_open(&fd, "grep");
    offset = 0;
    printf("inode %d \n", fd);
    size = file_read(fd, &offset, buf, 10);
    printf("Bytes read: %d \n", size);
    for (i = 0; i < size; i++) {
        if(buf[i]!='\0') putc(buf[i]);
    }
    file_close(&fd);
    return PASS;
}

/*
 * file_read_test4
 *   DESCRIPTION: test executable file - grep and show the tail
 *   INPUTS: none 
 *   OUTPUTS: The file data read from "grep"
 *   RETURN VALUE: PASS/FAIL
 */
int file_read_test4(){
    clear();
    TEST_HEADER;
    /* file descripter, reserved for cp3 */
    int32_t fd;
    /* a buffer to store the data */
    char buf[10000];
    int32_t size, i;
    uint32_t offset;
    file_open(&fd, "grep");
    offset = 0;
    printf("inode %d \n", fd);
    size = file_read(fd, &offset, buf, 10000);
    printf("Bytes read: %d \n", size);
    for (i = 0; i < size; i++) {
        if(buf[i]!='\0') putc(buf[i]);
    }
    file_close(&fd);
    return PASS;
}

/*
 * file_read_test5
 *   DESCRIPTION: test executable file - ls and show the head
 *   INPUTS: none 
 *   OUTPUTS: The file data read from "ls"
 *   RETURN VALUE: PASS/FAIL
 */
int file_read_test5(){
    clear();
    TEST_HEADER;
    /* file descripter, reserved for cp3 */
    int32_t fd;
    /* a buffer to store the data */
    char buf[10];
    int32_t size, i;
    uint32_t offset;
    file_open(&fd, "ls");
    offset = 0;
    printf("inode %d \n", fd);
    size = file_read(fd, &offset, buf, 10);
    printf("Bytes read: %d \n", size);
    for (i = 0; i < size; i++) {
        if(buf[i]!='\0') putc(buf[i]);
    }
    file_close(&fd);
    return PASS;
}

/*
 * file_read_test6
 *   DESCRIPTION: test executable file - ls and show the tail
 *   INPUTS: none 
 *   OUTPUTS: The file data read from "ls"
 *   RETURN VALUE: PASS/FAIL
 */
int file_read_test6(){
    clear();
    TEST_HEADER;
    /* file descripter, reserved for cp3 */
    int32_t fd;
    /* a buffer to store the data */
    char buf[10000];
    int32_t size, i;
    uint32_t offset;
    file_open(&fd, "ls");
    offset = 0;
    printf("inode %d \n", fd);
    size = file_read(fd, &offset, buf, 10000);
    printf("Bytes read: %d \n", size);
    for (i = 0; i < size; i++) {
        if(buf[i]!='\0') putc(buf[i]);
    }
    file_close(&fd);
    return PASS;
}

/*
 * file_read_separate_test1
 *   DESCRIPTION: test file_read to read frame1.txt twice separately
 *   INPUTS: none 
 *   OUTPUTS: The file data read from "frame1.txt"
 *   RETURN VALUE: PASS/FAIL
 */
int file_read_separate_test1(){
    // clear();
    TEST_HEADER;
    /* file descripter, reserved for cp3 */
    int32_t fd;
    /* a buffer to store the data */
    char buf[256];
    int32_t size, i;
    uint32_t offset;
    file_open(&fd, "frame1.txt");
    offset = 0;
    printf("inode %d \n", fd);
    size = file_read(fd, &offset, buf, 128);
    printf("read bytes %d \n", size);
    printf("offset %d \n", offset);
    size += file_read(fd, &offset, buf, 128);
    printf("read bytes %d \n", size);
    printf("offset %d \n", offset);
    printf("Bytes read: %d \n", size);
    for (i = 0; i < size; i++) {
        putc(buf[i]);
    }
    file_close(&fd);
    return PASS;
}

/*
 * file_read_separate_test2
 *   DESCRIPTION: test file_read to read frame0.txt and frame1.txt separately
 *   INPUTS: none 
 *   OUTPUTS: The file data read from "frame0.txt" and frame1.txt
 *   RETURN VALUE: PASS/FAIL
 */
int file_read_separate_test2(){
    // clear();
    TEST_HEADER;
    /* file descripter, reserved for cp3 */
    int32_t fd0;
    int32_t fd1;	
    /* a buffer to store the data */
    char buf0[256];
	char buf1[256];
    int32_t size0, size1, i, j;
    uint32_t offset0 = 0;
    uint32_t offset1 = 0;
    file_open(&fd0, "frame0.txt");
    file_open(&fd1, "frame1.txt");	
    printf("inode %d \n", fd0);
    size0 = file_read(fd0, &offset0, buf0, 64);
    printf("read bytes %d \n", size0);
    printf("offset %d \n", offset0);
    printf("inode %d \n", fd0);
    size1 = file_read(fd1, &offset1, buf1, 64);
    printf("read bytes %d \n", size1);
    printf("offset %d \n", offset1);
    printf("inode %d \n", fd1);
    for (i = 0; i < size0; i++) {
        putc(buf0[i]);
    }
    for (j = 0; j < size1; j++) {
        putc(buf1[j]);
    }
    file_close(&fd0);
    file_close(&fd1);
    return PASS;
}

/*
 * file_read_verylarge_test1
 *   DESCRIPTION: test file_read to read frame1.txt twice separately
 *   INPUTS: none 
 *   OUTPUTS: The file data read from "frame1.txt"
 *   RETURN VALUE: PASS/FAIL
 */
int file_read_verylarge_test1(){
    // clear();
    TEST_HEADER;
    /* file descripter, reserved for cp3 */
    int32_t fd;
    /* a buffer to store the data */
    char buf[10000];
    int32_t size, i;
    uint32_t offset;
    file_open(&fd, "verylargetextwithverylongname.txt");
    offset = 0;
    printf("inode %d \n", fd);
    size = file_read(fd, &offset, buf, 4096);
    printf("read bytes %d \n", size);
    printf("offset %d \n", offset);
    size += file_read(fd, &offset, buf, 4096);
    printf("read bytes %d \n", size);
    printf("offset %d \n", offset);
    printf("Bytes read: %d \n", size);
    for (i = 0; i < size; i++) {
        putc(buf[i]);
    }
    file_close(&fd);
    return PASS;
}

/*
 * dir_open_test1
 *   DESCRIPTION: open the target dir with the name "."
 *   INPUTS: none 
 *   OUTPUTS: index of the dir inode
 *   RETURN VALUE: PASS/FAIL
 */
int dir_open_test1(){
	// clear();
    TEST_HEADER;
    /* file descripter, reserved for cp3 */
    int32_t fd;
	if(dir_open(&fd, ".") == FAILURE) return FAIL;
	return PASS;
}

/*
 * dir_open_test2
 *   DESCRIPTION: open the target dir with the name "---"
 *   INPUTS: none 
 *   OUTPUTS: index of the dir inode
 *   RETURN VALUE: PASS/FAIL
 */
int dir_open_test2(){
	// clear();
    TEST_HEADER;
    /* file descripter, reserved for cp3 */
    int32_t fd;
	if(dir_open(&fd, "---") == FAILURE) return PASS;
	return FAIL;	
}

/*
 * dir_read_test1
 *   DESCRIPTION: read the direntry .
 *   INPUTS: none 
 *   OUTPUTS: cur
 *   RETURN VALUE: PASS/FAILURE
 */
int dir_read_test1(){
	// clear();
    TEST_HEADER;
    /* file descripter, reserved for cp3 */
    int32_t fd;
	uint32_t offset = 0;
	char buf[33];
	if(dir_open(&fd, ".") == FAILURE) return FAILURE;
	dir_read(fd, &offset, buf, 32);
	printf("The cur is %s \n", buf);
	return PASS;
}

/*
 * dir_read_test2
 *   DESCRIPTION: read the direntry .
 *   INPUTS: none 
 *   OUTPUTS: buff content
 *   RETURN VALUE: PASS/FAILURE
 */
//fs_start_ptr->num_inodes
int dir_read_test2(){
	// clear();
    TEST_HEADER;
    /* file descripter, reserved for cp3 */
    int32_t fd;
	uint32_t offset;
    int32_t i;
	if(dir_open(&fd, ".") == FAILURE) return FAILURE;
	for(i = 0; i < fs_start_ptr->num_dir_entries; i ++){
		char buf[33] = {'\0'*33};
		dir_read(fd, &offset, buf, 32);
		printf("%s \n", buf);
	}
	return PASS;
}

/*
 *rtc_cp2_testing
 *   DESCRIPTION: test the rtc in checkpoint2, it will print the number using different frequency.
 *   INPUTS: none 
 *   OUTPUTS: buff content
 *   RETURN VALUE: PASS
 */
int rtc_cp2_testing(){
	int test_fre[RTC_TEST_LEN] = {2, 4, 8, 16, 32, 64, 128, 256, 512, 1024};
	int i;
	int j;
	for(i = 0; i<= 9; i++){
		rtc_write(test_fre[i]);
		for(j = test_fre[i]-1; j>=0; j--){
			rtc_read();
			printf("%d", test_fre[i]);
		}
	}
    return PASS;
}

/*
 * scrolling_testing
 *   DESCRIPTION: test the scrolling function in putc.
 *   INPUTS: none 
 *   OUTPUTS: none
 *   RETURN VALUE: none 
 *   SIDE EFFECTS: test for scrolling the screen when the screen is full
 */
void scrolling_testing()
{
	TEST_HEADER;
	int i;
	for(i=0;i<=SCROLLING_TEST_MAX;i++)
	{
		printf("This is scrolling test: %d\n", i);
	}
}


/* Checkpoint 3 tests */

/*
 * test_file_operation1
 *   DESCRIPTION: do syscall inside kernel
 *   INPUTS: None
 *   OUTPUTS: None
 *   RETURN VALUE: None
 */
void test_file_operation1(){
    // first things about rtc
    cur_pid = 0;
    init_fda(0);
    int32_t fd;
    int32_t cnt = 10;
    uint8_t filename[] = "rtc";
    int32_t rtc_freq = 2;
    fd = open(filename);
    if (fd<=1){
        printf("Shit! Something wrong!\n");
    }
    printf("the fd is %d\n", fd);
    write(fd, &rtc_freq, 4);
    while (cnt){
        read(fd, NULL, 0);
        printf("2\n");
        cnt--;
    }
    close(fd);
    // try operate on some not-opened things
    read(fd, NULL, 0);
}

/*
 * test_file_operation2
 *   DESCRIPTION: do syscall inside kernel
 *   INPUTS: None
 *   OUTPUTS: None
 *   RETURN VALUE: None
 */
void test_file_operation2(){
    // first things about rtc
    cur_pid = 0;
    init_fda(0);
    int32_t fd_rtc, fd_fish, read_size, i;
    uint8_t buf[30];
    int32_t cnt = 20;
    uint8_t filename_rtc[] = "rtc";
    uint8_t filename_fish[] = "frame0.txt";
    int32_t rtc_freq = 4;
    fd_rtc = open(filename_rtc);
    if (fd_rtc<=1){
        printf("Shit! Something wrong!\n");
    }
    printf("the fd for rtc is %d\n", fd_rtc);
    write(fd_rtc, &rtc_freq, 4);
    // open fish frame
    fd_fish = open(filename_fish);
    if (fd_fish<=1){
        printf("Fuck! Something wrong!\n");
    }
    printf("the fd for fish is %d\n", fd_fish);
    // print things from file
    while (cnt){
        read(fd_rtc, NULL, 0);
        read_size = read(fd_fish, buf, 10);
        if(read_size>=0){
            //printf("read %d bytes\n", read_size);
            for (i=0; i<read_size; i++){
                putc(buf[i]);
            }
        }
        cnt--;
    }
    close(fd_rtc);
    close(fd_fish);
    // try operate on some not-opened things
    read(fd_rtc, NULL, 0);
    read(fd_fish, buf, 10);
}


/* Checkpoint 4 tests */
/* Checkpoint 5 tests */


/* Test suite entry point */
void launch_tests(){
    /* CHECKPOINT 1*/

    #if (CURRENT_CHECK_POINT==1)
    #if(RTC_ENABLE_PRINT==0)
    #if(CONTINUE_ERROR_TESTS)
    // continue testing with array as help
    	TEST_OUTPUT("idt_test", idt_test());
    	init_array();
    #if(TEST_WITH_ERROR)
     	push_back((uint32_t)selective_excp3);
     	push_back((uint32_t)selective_excp2);
     	push_back((uint32_t)selective_excp1);
    	push_back((uint32_t)paging_test4);
    	push_back((uint32_t)paging_test3);
    	push_back((uint32_t)paging_test2);
    	push_back((uint32_t)paging_test1);
    	push_back((uint32_t)div_by_error_test);
    	push_back((uint32_t)idt_test);
    #else
    	push_back((uint32_t)pic_enable_valid_test);
    	push_back((uint32_t)pic_enable_invalid_test);
    	push_back((uint32_t)pic_disable_invalid_test);
    	push_back((uint32_t)pic_disable_valid_test);
    	push_back((uint32_t)paging_test5);
    #endif
    	while (!is_empty()){
    		continue_testing();
    	}
    #else
    	idt_test();
    	div_by_error_test();
    	paging_test2();
    	paging_test5();
    	common_exception_tests(0x03);
    	pic_disable_invalid_test();
    	pic_disable_valid_test();
    	pic_enable_invalid_test();
    	pic_enable_valid_test();
    #endif
    #endif
    #endif

    /* CHECKPOINT 2*/
    #if(CURRENT_CHECK_POINT==2)
    /* rtc cp2 test block */

    // TEST_OUTPUT("rtc_test", rtc_cp2_testing());

    //test for scrolling the screen

	//scrolling_testing();		

	//test for terminal

	// uint8_t buf[MAX_TERMINAL_BUFFER];
	// uint8_t cnt = 100;          // we can set different number for different upper limit for tests.
	// uint32_t read_size;
	// terminal_open();
	// while (cnt){
	// 	read_size = terminal_read(0, buf, MAX_BUFFER_SIZE);
	// 	buf[read_size] = '\0';
	// 	terminal_write(1, buf, MAX_TERMINAL_BUFFER);
	// 	cnt--;
	// }
	// terminal_close();

    /* file read test block */

    // TEST_OUTPUT("file_read_test1", file_read_test1());
    // TEST_OUTPUT("file_read_test2", file_read_test2());
    // TEST_OUTPUT("file_read_test3", file_read_test3());
    // TEST_OUTPUT("file_read_test4", file_read_test4());
    // TEST_OUTPUT("file_read_test5", file_read_test5());
    // TEST_OUTPUT("file_read_test6", file_read_test6());
    // TEST_OUTPUT("file_read_separate_test1", file_read_separate_test1());
    // TEST_OUTPUT("file_read_separate_test2", file_read_separate_test2());
    // TEST_OUTPUT("file_read_verylarge_test1", file_read_verylarge_test1());

    /* read_dentry_by_name test block */

    // TEST_OUTPUT("read_dentry_by_valid_name_test", read_dentry_by_valid_name_test());
    // TEST_OUTPUT("read_dentry_by_invalid_name_test", read_dentry_by_invalid_name_test());
    // TEST_OUTPUT("read_dentry_by_dir_name_test", read_dentry_by_dir_name_test());
    
    /* read_dentry_by_index test block */

    // TEST_OUTPUT("read_dentry_by_valid_index_test", read_dentry_by_valid_index_test());
    // TEST_OUTPUT("read_dentry_by_invalid_index_test", read_dentry_by_invalid_index_test());

    /* dir open test block */

	// TEST_OUTPUT("dir_open_test1", dir_open_test1());
	// TEST_OUTPUT("dir_open_test2", dir_open_test2());
	 
	/* dir read test block */

	// TEST_OUTPUT("dir_read_test1", dir_read_test1());
	TEST_OUTPUT("dir_read_test2", dir_read_test2());
    #endif

    /* CHECKPOINT 3*/
    #if(CURRENT_CHECK_POINT==3)    
    // test syscall function without asm linkage
    // test_file_operation1();
    test_file_operation2();

    #endif
}
