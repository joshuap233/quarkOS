//
// Created by pjs on 2021/1/27.
//

#include "ps2.h"
#include "qstdint.h"
#include "x86.h"
#include "qlib.h"
#include "foo.h"
#include <stdbool.h>


bool poll_status(uint8_t bit, status_t expect) {
    //TODO: 定时轮询,添加 sleep 函数
/*
 *  轮询状态寄存器状态并等待,bit 为需要检查的位,expect 为期望值
 *  bit 范围: 0-7, 不会检查参数
 *  轮询结束没有达到期望值返回 False
*/
    for (int i = 0; i < N_POLL; ++i)
        if (((inb(PS2_CMD) >> bit) & 0b1) == expect)
            return true;
    return false;
}

// 轮询读取数据
uint8_t ps2_rd() {
    if (!poll_status(0, ZERO)) {
        printfk("ps2_rd error\n");
        panic();
    }
    return inb(PS2_DAT);
}

// 轮询写数据
void ps2_wd(uint8_t value) {
    if (!poll_status(1, ZERO)) {
        printfk("ps2_rd error\n");
        panic();
    }
    outb(PS2_DAT, value);
}

// 轮询写指令
void ps2_wc(uint8_t value) {
    if (!poll_status(1, 0)) {
        printfk("ps2_rd error\n");
    }
    outb(PS2_CMD, value);
}


void ps2_init() {
    //禁用 ps/2 两个端口
    ps2_wc(0xAD);
    ps2_wc(0xA7);

    //清空输出缓存
    ps2_rd();

    //读取控制位
    ps2_rd();
    uint8_t config = ps2_rd();
    //开启键盘中断
    set_bit(&config, 0);
    //关闭鼠标中断,关闭 scanCode set 转换
    clear_bit(&config, 1);
    clear_bit(&config, 6);

    //写回控制位
    ps2_wc(0x60);
    ps2_wd(config);

    //开启 ps/2 键盘端口
    ps2_wc(0xAE);

    //测试是否设置成功
    outb(PS2_CMD, 0xAA);
    assertk(inb(PS2_DAT) == 0x55);

    ps2_d_scode();
}

void ps2_d_device() {

}

void ps2_d_scode() {
    ps2_wc(0xD1);
    ps2_wd(0x1);
    printfk("ps2_scode: %x\n", ps2_rd());
}