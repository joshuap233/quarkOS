//
// Created by pjs on 2021/6/18.
//
// ethernet i217
//https://wiki.osdev.org/Intel_Ethernet_i217

#include <drivers/nic.h>
#include <types.h>


uint8_t read8(uint64_t p_address) {
    return *((volatile uint8_t *) (p_address));
}

uint16_t read16(uint64_t p_address) {
    return *((volatile uint16_t *) (p_address));

}

uint32_t read32(uint64_t p_address) {
    return *((volatile uint32_t *) (p_address));

}

uint64_t read64(uint64_t p_address) {
    return *((volatile uint64_t *) (p_address));
}

void write8(uint64_t p_address, uint8_t p_value) {
    (*((volatile uint8_t *) (p_address))) = (p_value);
}

void write16(uint64_t p_address, uint16_t p_value) {
    (*((volatile uint16_t *) (p_address))) = (p_value);
}

void write32(uint64_t p_address, uint32_t p_value) {
    (*((volatile uint32_t *) (p_address))) = (p_value);

}

void write64(uint64_t p_address, uint64_t p_value) {
    (*((volatile uint64_t *) (p_address))) = (p_value);
}