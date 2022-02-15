//
// Created by pjs on 2022/2/11.
//

#ifndef QUARKOS_MP_H
#define QUARKOS_MP_H

struct apic {
    u8_t nCpu;
    u8_t ioApicId;
    u32_t lapicPtr;  //lapic mmio 虚拟地址
    u32_t ioapicPtr; //ioapic mmio 虚拟地址
};

extern struct apic cpuCfg;

struct cpu *getCpu();

#endif //QUARKOS_MP_H
