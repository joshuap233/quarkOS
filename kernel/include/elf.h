//
// Created by pjs on 2021/2/21.
//
#ifndef QUARKOS_ELF_H
#define QUARKOS_ELF_H

#include <types.h>

typedef u16_t Elf32_Half;        // Unsigned half int
typedef u32_t Elf32_Off;         // Unsigned offset
typedef u32_t Elf32_Addr;        // Unsigned address
typedef u32_t Elf32_Word;        // Unsigned int
typedef int32_t Elf32_Sword;    // Signed int

/*-------- elf header ------*/

// e_ident
enum elf_ident {
    EI_MAG0 = 0, // 0x7F
    EI_MAG1 = 1, // 'E'
    EI_MAG2 = 2, // 'L'
    EI_MAG3 = 3, // 'F'
    EI_CLASS = 4, // Architecture (32/64)
    EI_DATA = 5, // Byte Order
    EI_VERSION = 6, // ELF Version
    EI_OSABI = 7, // OS Specific
    EI_ABIVERSION = 8, // OS Specific
    EI_PAD = 9  // Padding
};
# define ELF_MAG0        0x7F // e_ident[EI_MAG0]
# define ELF_MAG1        'E'  // e_ident[EI_MAG1]
# define ELF_MAG2        'L'  // e_ident[EI_MAG2]
# define ELF_MAG3    'F'  // e_ident[EI_MAG3]

# define ELF_DATA2LSB    1  // Little Endian
# define ELF_CLASS32    1  // 32-bit Architecture

// required architecture
# define EM_386            3  // x86 Machine Type
# define EV_CURRENT        1  // ELF Current Version

# define ELF_NIDENT     16

# define SHN_UNDEF        0 // Undefined/Not present

// object file type.
enum elf_type {
    ET_NONE = 0, // Unkown Type
    ET_REL = 1, // Relocatable File
    ET_EXEC = 2  // Executable File
};

typedef struct elf32_header {
    u8_t e_ident[ELF_NIDENT];
    Elf32_Half e_type;           // object file type.
    Elf32_Half e_machine;        // required architecture
    Elf32_Word e_version;        // object file version
    Elf32_Addr e_entry;          // virtual address to which the system first transfers control
    Elf32_Off e_phoff;           // 程序头表在文件中偏移
    Elf32_Off e_shoff;           // 节头表在文件中偏移(字节)
    Elf32_Word e_flags;
    Elf32_Half e_ehsize;         // ELF header's size in bytes
    Elf32_Half e_phentsize;      // 程序头表项大小
    Elf32_Half e_phnum;          // 程序头表项数
    Elf32_Half e_shentsize;      // 节头大小,节头为节头表项
    Elf32_Half e_shnum;          // 节头表项数量
    Elf32_Half e_shstrndx;       // strtab 在节头表中的索引
} elf32_header_t;

/*------------ section header table -------------*/
typedef struct elf32_sh {
    Elf32_Word sh_name;      // 名称在 string table 中的偏移
    Elf32_Word sh_type;
    Elf32_Word sh_flags;     // 节访问属性
    Elf32_Addr sh_addr;      // 该节第一个字节地址,不存在则为0
    Elf32_Off sh_offset;     // 该节首字节到文件首字节偏移
    Elf32_Word sh_size;      // 节大小
    Elf32_Word sh_link;
    Elf32_Word sh_info;
    Elf32_Word sh_addralign; // 节对齐要求
    Elf32_Word sh_entsize;   // 节中表项的长度,0表示无固定长度
} elf32_sh_t;


// elf32_sh_t.type
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


/*------- symbol table  --------*/
typedef struct elf32_symbol {
    Elf32_Word st_name;
    Elf32_Addr st_value;    // 可执行文件中,value 为虚拟地址
    Elf32_Word st_size;     // 符号对应的对象大小
    uint8_t st_info;        // 包括 symbol_bindings 与 symbol_types
    uint8_t st_other;       // 0
    Elf32_Half st_shndx;
} elf32_symbol_t;

//the first entry in each symbol table is a NULL entry
typedef struct elf32_symbol_table {
    elf32_symbol_t *header;
    size_t size;
    size_t entry_size;
} elf32_symbol_table_t;

#define ELF32_ST_BIND(INFO)    ((INFO) >> 4)
#define ELF32_ST_TYPE(INFO)    ((INFO) & MASK_U8(4))

enum symbol_bindings {
    STB_LOCAL = 0,       // Local scope
    STB_GLOBAL = 1,      // Global scope
    STB_WEAK = 2         // Weak, (ie. __attribute__((weak)))
};


enum symbol_types {
    STT_NOTYPE = 0,     // No type
    STT_OBJECT = 1,     // Variables, arrays, etc.
    STT_FUNC = 2      // Methods or functions
};


/*----- string table ------*/
/*
 * string table  为一连串字符串,以 '\0' 分隔
 * 且 string table 第一个字节为 '\0'
 */

/*----- program header------*/

/*
 * program header table is an array of structures,
 * each describing a segment or other information the system needs to
 * prepare the program for execution
 */
typedef struct elf32_phdr{
    Elf32_Word p_type;
    Elf32_Off p_offset;   // 节的第一个字节到文件头的偏移
    Elf32_Addr p_vaddr;
    Elf32_Addr p_paddr;
    Elf32_Word p_filesz;  // 文件中段的大小
    Elf32_Word p_memsz;   // 内存中段大小, bss 段中,p_filesz 的值与 p_memsz 不同
    Elf32_Word p_flags;
    Elf32_Word p_align;
} elf32_phdr_t;

// elf_program_header.p_type
enum segment_type {
    PT_NULL = 0, // ignore
    PT_LOAD = 1,
    PT_DYNAMIC = 2,
    PT_INTERP = 3,
    PT_NOTE = 4,
    PT_SHLIB = 5,
    PT_PHDR = 6,
    PT_LOPROC = 0x70000000,
    PT_HIPROC = 0x7fffffff
};

enum segment_flag {
    PF_X = 0x1,    // Execute
    PF_W = 0x2,    // Write
    PF_R = 0x4,    // Read
    PF_MASKPROC = 0xf0000000
};


#endif //QUARKOS_ELF_H
