//
// Created by pjs on 2021/1/25.
//
#include "lib/qlib.h"
#include "isr.h"

//页错误错误码
typedef struct pf_error_code {
    uint16_t p: 1;   //置 0 则异常由页不存在引起,否则由特权级保护引起
    uint16_t w: 1;   //置 0 则访问是读取
    uint16_t u: 1;   //置 0 则特权模式下发生的异常
    uint16_t r: 1;
    uint16_t i: 1;
    uint16_t pk: 1;
    uint16_t zero: 10;
    uint16_t sgx: 1;
    uint16_t zero1: 15;
}PACKED pf_error_code_t;


// 带 '#' 符号为相应的中断向量的速记, 括号内为引发的指令
// [y] 表示有错误码
INT ISR(0)(interrupt_frame_t *);       // #DE 除数为0(DIV,IDIV)
INT ISR(1)(interrupt_frame_t *);       // #DB debug 异常
INT ISR(2)(interrupt_frame_t *);       // NMI 不可屏蔽中断
INT ISR(3)(interrupt_frame_t *);       // #BP 断点(INT 3)
INT ISR(4)(interrupt_frame_t *);       // #OF 溢出(INTO)
INT ISR(5)(interrupt_frame_t *);       // #BR 数组的引用超出边界(BOUND)
INT ISR(6)(interrupt_frame_t *);       // #UD 无效或未定义操作码(UD2)
INT ISR(7)(interrupt_frame_t *);       // #NM 没有数学协处理器
INT ISR(8)(interrupt_frame_t *, uint32_t);       // #DF double fault [y]
INT ISR(9)(interrupt_frame_t *);       // 协处理器段超限
INT ISR(10)(interrupt_frame_t *, uint32_t);      // #TS 无效TSS [y]
INT ISR(11)(interrupt_frame_t *, uint32_t);      // #NP 段不存在 [y]
INT ISR(12)(interrupt_frame_t *, uint32_t);      // #SS 栈段错误 [y]
INT ISR(13)(interrupt_frame_t *, uint32_t);      // #GP General Protection [y]
INT ISR(14)(interrupt_frame_t *, uint32_t);      // #PF 页错误[y]
// 15 不要使用
INT ISR(16)(interrupt_frame_t *);      // 16 #MF x87 FPU Floating-Point Error
INT ISR(17)(interrupt_frame_t *, uint32_t);      // 17 #AC 对齐检查 [y]
INT ISR(18)(interrupt_frame_t *);      // 18 #MC 机器检查
INT ISR(19)(interrupt_frame_t *);      // 19 #XM SIMD Floating-Point Exception
INT ISR(20)(interrupt_frame_t *);      // 20 #VE Virtualization Exception
//21-31 不要使用
//32-255 用户定义


typedef struct idtr {
    uint16_t limit;
    uint32_t address;
}PACKED idtr_t;


// gate 描述符
struct gate_desc {
    uint16_t offset_l; // 处理程序入口偏移 0-15位
    uint16_t selector; // 目标代码段的段选择子
    uint8_t zero;      // 保留
    uint8_t type;
    uint16_t offset_h; // 偏移 16-31 位
}PACKED;

typedef struct gate_desc interrupt_gate_t;
typedef struct gate_desc trap_gate_t;

#define IDT_SIZE   sizeof(union idt)
#define IDT_COUNT  256
#define IDT_TYPE   0b10001110   // 中断门,32 位,特权级为 0
#define IDT_UTYPE  0b11101110   // 用户级中断门
#define IDT_SC     0x08         // 代码段选择子

typedef union idt {
    interrupt_gate_t interrupt_gate;
    trap_gate_t trap_gate;
}PACKED idt_t;

extern void idtr_set(uint32_t idtr);
static idt_t _Alignas(8) idt[IDT_COUNT] = {0};
extern void syscall_entry();

// 设置 interrupt gate
void idt_set_ig(idt_t *entry, uint32_t offset, uint16_t selector, uint8_t type) {
    interrupt_gate_t *ig = &(entry->interrupt_gate);
    ig->zero = 0;
    ig->type = type;
    ig->selector = selector;
    ig->offset_h = offset >> 16;
    ig->offset_l = offset & MASK_U32(16);
}


void idt_init() {
    idtr_t idtr = {
            .limit = IDT_SIZE * IDT_COUNT - 1,
            .address = (ptr_t) idt
    };
    //暂时全部使用 interrupt gate(调用 ISR 时自动清除 IF 位)
    reg_isr(0, ISR(0));
    reg_isr(1, ISR(1));
    reg_isr(2, ISR(2));
    reg_isr(3, ISR(3));
    reg_isr(4, ISR(4));
    reg_isr(5, ISR(5));
    reg_isr(6, ISR(6));
    reg_isr(7, ISR(7));
    reg_isr(8, ISR(8));
    reg_isr(9, ISR(9));
    reg_isr(10, ISR(10));
    reg_isr(11, ISR(11));
    reg_isr(12,ISR(12));
    reg_isr(13, ISR(13));
    reg_isr(14, ISR(14));
    reg_isr(16, ISR(16));
    reg_isr(17, ISR(17));
    reg_isr(18, ISR(18));
    reg_isr(19, ISR(19));
    reg_isr(20, ISR(20));

    // 系统调用中断
    idt_set_ig(&idt[0x80], (ptr_t) syscall_entry, IDT_SC, IDT_UTYPE);
    idtr_set((ptr_t) &idtr);
}

void reg_isr(uint8_t n,void *isr){
    idt_set_ig(&idt[n], (ptr_t) isr, IDT_SC, IDT_TYPE);
}

// 忽略 -Wunused-parameter
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"


// 编译器会保留 EFLAGS 外的所有寄存器,并使用 iret 返回
INT ISR(0)(interrupt_frame_t *frame) {
    panic();
}

INT ISR(1)(interrupt_frame_t *frame) {
    panic();
}

INT ISR(2)(interrupt_frame_t *frame) {
    panic();
}

INT ISR(3)(interrupt_frame_t *frame) {
}

INT ISR(4)(interrupt_frame_t *frame) {
    panic();
}

INT ISR(5)(interrupt_frame_t *frame) {
    panic();
}

INT ISR(6)(interrupt_frame_t *frame) {
    panic();
}

INT ISR(7)(interrupt_frame_t *frame) {
    panic();
}

INT ISR(8)(interrupt_frame_t *frame, uint32_t error_code) {
    UNUSED error_code_t *ec = (error_code_t *) (&error_code);
}

INT ISR(9)(interrupt_frame_t *frame) {
    panic();
}

INT ISR(10)(interrupt_frame_t *frame, uint32_t error_code) {
    panic();
}

INT ISR(11)(interrupt_frame_t *frame, uint32_t error_code) {
    panic();
}

INT ISR(12)(interrupt_frame_t *frame, uint32_t error_code) {
    panic();
}

INT ISR(13)(interrupt_frame_t *frame, uint32_t error_code) {
    error_code_t *ec = (error_code_t *) &error_code;
    assertk(ec->zero == 0);
    printfk("GP exception\n");
    printfk("gp error code: %u\n", error_code);
    panic();
}

INT ISR(14)(interrupt_frame_t *frame, uint32_t error_code) {
//页错误
    pf_error_code_t *ec = (pf_error_code_t *) &error_code;
    assertk(ec->zero == 0);
    assertk(ec->zero1 == 0);
    printfk("page fault, error code: %x\n", error_code);
    printfk("page fault addr: %x\n", pf_addr());
    panic();
}


INT ISR(16)(interrupt_frame_t *frame) {
    panic();
}

INT ISR(17)(interrupt_frame_t *frame, uint32_t error_code) {
    panic();
}

INT ISR(18)(interrupt_frame_t *frame) {
    panic();
}

INT ISR(19)(interrupt_frame_t *frame) {
    panic();
}

INT ISR(20)(interrupt_frame_t *frame) {
    panic();
}


#pragma GCC diagnostic pop
