//
// Created by pjs on 2021/1/27.
//

#include <stdbool.h>
#include "ps2.h"
#include "types.h"
#include "x86.h"
#include "qlib.h"
#include "keyboard.h"

bool poll_status(uint8_t bit, status_t expect) {
/*
 *  轮询状态寄存器状态并等待,bit 为需要检查的位,expect 为期望值
 *  bit 范围: 0-7, 不会检查参数
 *  轮询结束没有达到期望值返回 False
*/
    for (int i = 0; i < N_POLL; ++i)
        if (ps2_cs(bit, expect)) {
            ssleep(10);
            return true;
        }
    return false;
}

// 轮询读取数据
uint8_t ps2_prd() {
    if (!poll_status(0, ZERO)) {
        printfk("ps2_rd error\n");
        panic();
    }
    return ps2_rd();
}

// 轮询写数据
void ps2_pwd(uint8_t value) {
    if (!poll_status(1, ZERO)) {
        printfk("ps2_rd error\n");
        panic();
    }
    ps2_wd(value);
}

// 轮询写指令
void ps2_pwc(uint8_t value) {
    if (!poll_status(1, ZERO)) {
        printfk("ps2_rd error\n");
    }
    ps2_wc(value);
}


void ps2_init() {
    //禁用 ps/2 两个端口
    ps2_pwc(0xAD);
    ps2_pwc(0xA7);

    //清空输出缓存
    ps2_rd();

    //读取控制位
    ps2_prd();
    uint8_t config = ps2_prd();
    //开启键盘中断
    set_bit(&config, 0);
    //关闭鼠标中断,关闭 scanCode set 转换
    clear_bit(&config, 1);
    clear_bit(&config, 6);

    //写回控制位
    ps2_pwc(0x60);
    ps2_pwd(config);

    //测试端口1
    ps2_wc(0xAB);
    assertk(ps2_rd() == 0x00);

    //开启 ps/2 键盘端口1
    ps2_pwc(0xAE);
    //测试是否设置成功,会接受到中断
    ps2_wc(0xAA);
    assertk(ps2_rd() == 0x55);

    //初始化 ps/2 键盘
    kb_init();
}


uint8_t ps2_device_cmd(uint8_t cmd, device_status_t ds) {
    uint8_t res;
    ps2_pwd(cmd);
    while ((res = ps2_prd()) == ds.resend);
    return res;
}
