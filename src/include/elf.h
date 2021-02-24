//
// Created by pjs on 2021/2/21.
//
//https://wiki.osdev.org/ELF_Tutorial

#ifndef QUARKOS_ELF_H
#define QUARKOS_ELF_H

#include "types.h"
#include "klib/qlib.h"

typedef uint16_t Elf32_Half;    // Unsigned half int
typedef uint32_t Elf32_Off;     // Unsigned offset
typedef uint32_t Elf32_Addr;    // Unsigned address
typedef uint32_t Elf32_Word;    // Unsigned int
typedef int32_t Elf32_Sword;    // Signed int

# define ELF_NIDENT    16
typedef struct elf32_header {
    uint8_t e_ident[ELF_NIDENT];
    Elf32_Half e_type;
    Elf32_Half e_machine;
    Elf32_Word e_version;
    Elf32_Addr e_entry;
    Elf32_Off e_phoff;
    Elf32_Off e_shoff;
    Elf32_Word e_flags;
    Elf32_Half e_ehsize;
    Elf32_Half e_phentsize;
    Elf32_Half e_phnum;
    Elf32_Half e_shentsize;
    Elf32_Half e_shnum;
    Elf32_Half e_shstrndx;
} elf32_header_t;

// sh -> section header
typedef enum sh_types {
    SHT_NULL = 0,       // Null section
    SHT_PROGBITS = 1,   // Program information
    SHT_SYMTAB = 2,     // Symbol table
    SHT_STRTAB = 3,     // String table,包括 strtab,shstrtab(读取节名)
    SHT_RELA = 4,       // Relocation (w/ addend)
    SHT_NOBITS = 8,     // Not present in file
    SHT_REL = 9,        // Relocation (no addend)
} sh_types_t;

// elf32_sh_t.flags 标志位
typedef enum sh_attr {
    SHF_WRITE = 0x01, // Writable section
    SHF_ALLOC = 0x02  // Exists in memory
} sh_attr_t;

typedef struct elf32_sh {
    Elf32_Word sh_name;    //名称在 string table 中的偏移
    Elf32_Word sh_type;
    Elf32_Word sh_flags;   //节访问属性
    Elf32_Addr sh_addr;    //该节第一个字节地址,不存在则为0
    Elf32_Off sh_offset;  //该节首字节到文件首字节偏移
    Elf32_Word sh_size;
    Elf32_Word sh_link;
    Elf32_Word sh_info;
    Elf32_Word sh_addralign; //节对齐要求
    Elf32_Word sh_entsize;   //节中表项的长度,0表示无固定长度
} elf32_sh_t;


typedef struct elf32_symbol {
    Elf32_Word st_name;
    Elf32_Addr st_value;
    //不同的类型文件(可重定向,可执行),value有不同含义
    //可执行文件中,value 为虚拟地址
    Elf32_Word st_size; //符号对应的对象大小
    uint8_t st_info;  //包括 StT_Bindings 与 StT_Types
    uint8_t st_other; //0
    Elf32_Half st_shndx;
} elf32_symbol_t;


enum StT_Bindings {
# define ELF32_ST_BIND(INFO)    ((INFO) >> 4)
    STB_LOCAL = 0, // Local scope
    STB_GLOBAL = 1, // Global scope
    STB_WEAK = 2  // Weak, (ie. __attribute__((weak)))
};


enum StT_Types {
# define ELF32_ST_TYPE(INFO)    ((INFO) & MASK_U8(4))
    STT_NOTYPE = 0, // No type
    STT_OBJECT = 1, // Variables, arrays, etc.
    STT_FUNC   = 2  // Methods or functions
};

typedef struct elf_string_table {
    char *addr; //header[0] == '\0'
    size_t size;
} elf_string_table_t;

typedef struct elf_symbol_table {
    elf32_symbol_t *header;
    size_t size;
    size_t entry_size;
} elf_symbol_table_t;


#endif //QUARKOS_ELF_H
