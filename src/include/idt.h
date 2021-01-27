#ifndef QUARKOS_IDT_H
#define QUARKOS_IDT_H

#include "qstdint.h"

typedef struct idtr {
    uint16_t limit;
    uint32_t address;
}__attribute__((packed)) idtr_t;


// gate 描述符
struct gate_desc {
    uint16_t offset_l; // 处理程序入口偏移 0-15位
    uint16_t selector; // 目标代码段的段选择子
    uint8_t zero;      // 保留
    uint8_t type;
    uint16_t offset_h; // 偏移 16-31 位
}__attribute__((packed));

typedef struct gate_desc interrupt_gate_t;
typedef struct gate_desc trap_gate_t;

typedef union idt {
    interrupt_gate_t interrupt_gate;
    trap_gate_t trap_gate;
#define IDT_SIZE sizeof(union idt)
#define IDT_COUNT 256
#define IDT_TYPE 0b10001110 //interrupt_gate type 32 位,特权级为 0
#define IDT_INT_USR_TYPE32 0b11101110
#define IDT_SC  0x08     //代码段选择子
}__attribute__((packed)) idt_t;

void idt_init();

typedef struct interrupt_frame {
    uint32_t eip;
    uint16_t cs;
    uint32_t flags: 20;
    uint32_t zero: 12;
    // 特权级切换时会入栈
    uint16_t esp;
    uint16_t ss;
}__attribute__((packed)) interrupt_frame_t;


typedef struct error_code {
    uint16_t ext: 1;
    uint16_t idt: 1;
    uint16_t ti: 1;
    uint16_t ssi: 13;
    uint16_t zero;
}__attribute__((packed)) error_code_t;


#define ISR(index) isr ## index

// 带 '#' 符号为相应的中断向量的速记, 括号内为引发的指令
// [y] 表示有错误码
void ISR(0)(interrupt_frame_t *);       // #DE 除数为0(DIV,IDIV)
void ISR(1)(interrupt_frame_t *);       // #DB debug 异常
void ISR(2)(interrupt_frame_t *);       // NMI 不可屏蔽中断
void ISR(3)(interrupt_frame_t *);       // #BP 断点(INT 3)
void ISR(4)(interrupt_frame_t *);       // #OF 溢出(INTO)
void ISR(5)(interrupt_frame_t *);       // #BR 数组的引用超出边界(BOUND)
void ISR(6)(interrupt_frame_t *);       // #UD 无效或未定义操作码(UD2)
void ISR(7)(interrupt_frame_t *);       // #NM 没有数学协处理器
void ISR(8)(interrupt_frame_t *, uint32_t);       // #DF double fault [y]
void ISR(9)(interrupt_frame_t *);       // 协处理器段超限
void ISR(10)(interrupt_frame_t *, uint32_t);      // #TS 无效TSS [y]
void ISR(11)(interrupt_frame_t *, uint32_t);      // #NP 段不存在 [y]
void ISR(12)(interrupt_frame_t *, uint32_t);      // #SS 栈段错误 [y]
void ISR(13)(interrupt_frame_t *, uint32_t);      // #GP General Protection [y]
void ISR(14)(interrupt_frame_t *, uint32_t);      // #PF 页错误[y]
// 15 不要使用
void ISR(16)(interrupt_frame_t *);      // 16 #MF x87 FPU Floating-Point Error
void ISR(17)(interrupt_frame_t *, uint32_t);      // 17 #AC 对齐检查 [y]
void ISR(18)(interrupt_frame_t *);      // 18 #MC 机器检查
void ISR(19)(interrupt_frame_t *);      // 19 #XM SIMD Floating-Point Exception
void ISR(20)(interrupt_frame_t *);      // 20 #VE Virtualization Exception
//21-31 不要使用
//32-255 用户定义

void ISR(32)(interrupt_frame_t *);      // 20 #VE Virtualization Exception

extern void idtr_set(uint32_t idtr);

#endif //QUARKOS_IDT_H
