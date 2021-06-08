//
// Created by pjs on 2021/6/4.
//
// TODO:使用 SYSENTER/SYSEXIT?
// 系统调用使用 ax 寄存器存取处理函数编号

#include <types.h>
#include <syscall/syscall.h>
#include <isr.h>
#include <fs/vfs.h>
#include <sched/fork.h>


// 忽略 -Wunused-parameter
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"

// 传递系统调用参数
struct sys_reg {
    u32_t eax;  // 系统调用编号
    u32_t ebx;
    u32_t ecx;
    u32_t edx;
    u32_t esi;
    u32_t edi;
};


int sys_exec(u32_t *args) {

    return 0;
}

int sys_fork(u32_t *args) {
    return kernel_fork();
}

int sys_getchar(u32_t *args) {

    return 0;
}

int sys_putchar(u32_t *args) {
    return 0;

}

int sys_mkdir(u32_t *args) {
    return 0;

}

int sys_open(u32_t *args) {
    return 0;

}

int sys_close(u32_t *args) {
    return 0;

}

int sys_mkfile(u32_t *args) {
    return 0;

}

int sys_rmdir(u32_t *args) {
    return 0;

}

int sys_unlink(u32_t *args) {
    return 0;

}


int sys_read(u32_t *args) {
    return 0;

}

int sys_write(u32_t *args) {
    return 0;

}

int sys_sleep(u32_t *args) {
    return 0;
}

int sys_exit(u32_t *args){
    task_exit();
    return 0;
}

static int (*syscall[])(u32_t *args) = {
        [SYS_EXEC] = sys_exec,
        [SYS_GETCHAR] = sys_getchar,
        [SYS_PUTCHAR] = sys_putchar,
        [SYS_MKDIR] = sys_mkdir,
        [SYS_MKFILE] = sys_mkfile,
        [SYS_RMDIR] = sys_rmdir,
        [SYS_UNLINK] = sys_unlink,
        [SYS_OPEN] = sys_open,
        [SYS_CLOSE] = sys_close,
        [SYS_FORK] = sys_fork,
        [SYS_READ] = sys_read,
        [SYS_WRITE] = sys_write,
        [SYS_SLEEP] = sys_sleep,
        [SYS_EXIT] = sys_exit
};


int syscall_isr(struct sys_reg reg) {
    // 系统调用
    u32_t no = reg.eax;
    u32_t args[5];
    if (no <= 9) {
        args[0] = reg.ebx;
        args[1] = reg.ecx;
        args[2] = reg.edx;
        args[3] = reg.esi;
        args[4] = reg.edi;
        return syscall[no](args);
    }
    return -1;
}

#pragma GCC diagnostic pop
