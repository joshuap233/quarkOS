//
// Created by pjs on 2021/1/27.
//
#include "types.h"
#include "drivers/ps2.h"
#include "x86.h"
#include "klib/qlib.h"


typedef bool status_t;
#define FULL     1
#define EMPTY    0
#define N_POLL     10  //轮询端口次数


bool poll_status(uint8_t bit, status_t expect);

void ps2_pwc(uint8_t value);

//poll write data
void ps2_pwd(uint8_t value);

uint8_t ps2_prd();


//读取状态寄存器
INLINE uint8_t ps2_rs() {
    return inb(PS2_CMD);
}


INLINE void ps2_wd(uint8_t value) {
    outb(PS2_DAT, value);
}

INLINE void ps2_wc(uint8_t value) {
    outb(PS2_CMD, value);
}

//检测状态位
INLINE bool ps2_cs(uint8_t bit, status_t expect) {
    return ((ps2_rs() >> bit) & 0b1) == expect;
}

bool poll_status(uint8_t bit, status_t expect) {
/*
 *  轮询状态寄存器状态并等待,bit 为需要检查的位,expect 为期望值
 *  bit 范围: 0-7, 不会检查参数
*/
    for (int i = 0; i < N_POLL; ++i) {
        if (ps2_cs(bit, expect)) {
            return true;
        }
        io_wait();
    }
    return false;
}

// 轮询读取数据
uint8_t ps2_prd() {
    assertk(poll_status(0, FULL));
    return ps2_rd();
}

// 轮询写数据
void ps2_pwd(uint8_t value) {
    assertk(poll_status(1, EMPTY));
    ps2_wd(value);
}

// 轮询写指令
void ps2_pwc(uint8_t value) {
    assertk(poll_status(1, EMPTY));
    ps2_wc(value);
}


void ps2_init() {
    //禁用 ps/2 两个端口
    ps2_pwc(0xAD);
    ps2_pwc(0xA7);

    //清空输出缓存
    ps2_rd();

    //读取控制位
    ps2_pwc(0x20);
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


}


uint8_t ps2_device_cmd(uint8_t cmd, device_status_t ds) {
    uint8_t res;
    ps2_pwd(cmd);
    while ((res = ps2_prd()) == ds.resend);
    return res;
}
