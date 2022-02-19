//
// Created by pjs on 2022/2/12.
//

#ifndef QUARKOS_DRIVERS_ACPI_H
#define QUARKOS_DRIVERS_ACPI_H

//System Description Table 标准头
struct sdtHeader {
    char signature[4];
    uint32_t length;
    uint8_t version;
    uint8_t checksum;
    char oemId[6];
    char opemTableId[8];
    uint32_t oemVision;
    uint32_t creatorID;
    uint32_t creatorVersion;
}PACKED;

struct sysDesTable {
    void *madt;
};

bool acpiChecksum(struct sdtHeader *header);

extern struct sysDesTable sysDesTable;

#endif //QUARKOS_DRIVERS_ACPI_H
