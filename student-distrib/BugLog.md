# Bug Log

TEAM: 

Lian Xinyu

Liu Tianyu

Lou Jiaqi

Zhu Zhongbo

## CP1_1: printf

Reported by: Lian Xinyu

#### Summary

When modifying the putc function in "lib.c" to solve the full-screen problem, when I enter a newline, the position of the new line will always at the bottom of the windows, which is really strange.

#### Steps to reproduce

if((screen_x == NUM_COLS-1) && (screen_y = NUM_ROWS-1)) -> if((screen_x == NUM_COLS-1) && (screen_y == NUM_ROWS-1))

#### What is the current bug behavior?

When entering a new line, the new position will always at the bottom of the windows.


#### What is the expected correct behavior?

The new position should follow the current line. 

#### Possible fixes

Pay attention to the type! It is a deadly bug because no warnings when compiling.

#### Time spend
1.5h



## CP1_2: paging

Reported by: Zhu Zhongbo 

#### Summary

The paging cannot work and also crash the system. The gdb failed to work because of some seg-fault.

#### Possible fixes

The reason why the gdb cannot work is that after target remote IP, we have to use "continue" instead of "run". The reason why the paging cannot work is that I defined the PDE in a wrong way. I should have defined the 4KB page and 4MB page in different ways. So I changed the definition of struct and the base address for the pde should be 20bit.

#### Time spend

6h



## CP1_3: assembly file type

Reported by: Liu Tianyu

#### Summary

If we write the file name "interrupt_saveregister.s" rather than "interrupt_saveregister.S", we will receive an error message "undefined interrupt_handler function".

#### Steps to reproduce

Change "interrupt_saveregister.S" to "interrupt_saveregister.s"

#### What is the current bug behavior?

An error message "undefined interrupt_handler function".

#### Possible fixes

Pay attention to the difference between .s and .S

**.s** assembly language source program;Action: Assembler
**.S** assembly language source program;Operation: preprocessing + assembly (preprocessing: #define, #include, etc.)


## CP1_4: PIC mask not change globally

Reported by: Jiaqi Lou

#### Summary

When testing keyboard and rtc together, it is found that the rtc irq is not enabled.

#### Steps to reproduce

Not use a local variable as the data transmitted to PIC's, instead, using the global variable that allows multiple irq to be enabled at the same time.

#### What is the current bug behavior?

Both keyboard and rtc irq are enabled after initialization.

#### Possible fixes

Directly modify the global variables, master_mask and slave_mask.



## CP2_1: File system read_dentry_by_name function raises paging fault

Reported by: Jiaqi Lou

#### Summary

When calling read_dentry_by_name with "verylargetextwithverylongname.txt" file, the paging fault always raises.

#### Steps to reproduce

The "strlen" function in lib.c stops looping when a '\0' is found. But if the file name stored in dentry not containing a '\0', it will keep looping and cause a paging fault. 

#### What is the current bug behavior?

"verylargetextwithverylongname.txt" file can be accessed successfully.

#### Possible fixes

Preprocess the file name passed to this function. Use a buf[33] to truncate the file name and add '\0' to the end of the buf (that is, buf[32]).


## CP2_2: File content printed to screen is not correct

Reported by: Jiaqi Lou

#### Summary

When print the file content to the screen, I found that the content had some blanks.

#### Steps to reproduce

The contents with characters are correct but there are some blanks and the later part of the file is missing. This means that I may put the contents in wrong position in the buffer.

#### Possible fixes

When I did type cast, I should take care that only the pointer should be casted. For example, (char*) buf + nbytes instead of (char*) (buf + nbytes).



## CP3_1: inifite print of ls

Reported by: Zhongbo Zhu, Tianyu Liu

#### Summary

The "ls" prints "." all the time.

### Possible fixes

Forget to update the file pos in the file_descriptor array when we are reading a directory. 



## CP3_2: cannot deal with invalid command

Reported by: Zhongbo Zhu

#### Summary

Type in some string like "sss", then we get a page fault. 

### What is the expected correct behavior?

The shell prints "invalid command" and go on. 

### Possible fixes

If we fail in the step of loading things into memory, we have to turn the paging for user program back to shell. 



## CP3_3: cannot pass syserr test 4

Reported by: Tianyu Liu

#### Summary

Can open file "shel" even though it is not exist in our file system.

### What is the expected correct behavior?

We should let the shell print "no such file".

### Possible fixes

When we are comparing the strings, things like compare("shell", "shel", 4) is going to pass because the first four char are the same. So we have to do things like compare("shell", "shel", MAX_FILENAME_LEN) so that the '\0' at the end is taken in consideration. 



## CP3_4: cannot pass syserr test 5

Reported by: Tianyu Liu

#### Summary

The syserr test failed and we can somehow close an unopened file.

### What is the expected correct behavior?

Didn't clean all the opened files (including stdin&stdout) in the last execution. 

### Possible fixes

Close all the opened files in halt. Add the condition "file->flags==FILE_FLAG_FREE" which will return -1.



## CP3_5: exit base shell fail

Reported by: Zhongbo Zhu, Jiaqi Lou

#### Summary

Exiting base shell will lead to page fault

### What is the expected correct behavior?

Do not leave the base shell and print warning. 

### Possible fixes

Set the cur_pid to -1 when the user is trying to close the base shell and run execute("shell") again to restar the base shell.


## CP3_6: Wrong EIP position

Reported by: Zhongbo Zhu, Tianyu Liu

#### Summary

Cannot run execute because wrong EIP position

### What is the expected correct behavior?

Should set the pointer as unit8_t rather than uint32_t because of the 32 bits system.

### Possible fixes

change uint32_t* to uint8_t*.



