//
// Created by pjs on 2021/4/6.
//

#ifndef QUARKOS_DRIVERS_INIT_H
#define QUARKOS_DRIVERS_INIT_H

void ide_init();
void kb_init();
void pic_init(uint32_t offset1, uint32_t offset2);
void vga_init();
void ps2_init();
void pit_init(uint32_t frequency);
void gdt_init();


#endif //QUARKOS_DRIVERS_INIT_H
