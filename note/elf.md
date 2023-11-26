![2021-02-21 17-22-25 的屏幕截图](/home/pjs/Pictures/2021-02-21%2017-22-25%20%E7%9A%84%E5%B1%8F%E5%B9%95%E6%88%AA%E5%9B%BE.png)

实际的节排序可以使用链接脚本指定



Symbol Table:

An object file's symbol table holds information needed to locate and relocate a program's symbolic definitions and references.  A symbol table index is a subscript into this array. Index 0 both designates the first entry in the table and serves as the undefined symbol index. The contents of the initial entry are specified later in this section



节头表包含其他节头信息

![2021-06-15 08-49-04 的屏幕截图](image/2021-06-15%2008-49-04%20%E7%9A%84%E5%B1%8F%E5%B9%95%E6%88%AA%E5%9B%BE.png)

readelf -h 读取 elf 头

readelf -S 读取节头表



```
❯ i686-elf-readelf -S quarkOS.bin
There are 21 section headers, starting at offset 0x4f8a4:

Section Headers:
  [Nr] Name              Type            Addr     Off    Size   ES Flg Lk Inf Al
  [ 0]                   NULL            00000000 000000 000000 00      0   0  0
  [ 1] .init.text        PROGBITS        00100000 001000 0000fa 00  AX  0   0  8
  [ 2] .init.data        PROGBITS        00101000 002000 004010 00  WA  0   0 4096
  [ 3] .user.text        PROGBITS        00106000 007000 00007a 00  AX  0   0  1
  [ 4] .text             PROGBITS        c0107000 008000 00f32e 00  AX  0   0 16
  [ 5] .rodata           PROGBITS        c0117000 018000 0015ad 00   A  0   0 32
  [ 6] .eh_frame         PROGBITS        c01185b0 0195b0 0000b0 00   A  0   0  4
  [ 7] .data             PROGBITS        c0119000 01a000 0010f8 00  WA  0   0 32
  [ 8] .bss              NOBITS          c011b000 01b0f8 109d94 00  WA  0   0 4096
  [ 9] .debug_info       PROGBITS        00000000 01b0f8 019728 00      0   0  1
  [10] .debug_abbrev     PROGBITS        00000000 034820 0054ca 00      0   0  1
  [11] .debug_aranges    PROGBITS        00000000 039cf0 000610 00      0   0  8
  [12] .debug_ranges     PROGBITS        00000000 03a300 000118 00      0   0  1
  [13] .debug_line       PROGBITS        00000000 03a418 00a62d 00      0   0  1
  [14] .debug_str        PROGBITS        00000000 044a45 003274 01  MS  0   0  1
  [15] .comment          PROGBITS        00000000 047cb9 000012 01  MS  0   0  1
  [16] .debug_frame      PROGBITS        00000000 047ccc 003058 00      0   0  4
  [17] .debug_loc        PROGBITS        00000000 04ad24 000d87 00      0   0  1
  [18] .symtab           SYMTAB          00000000 04baac 002810 10     19 350  4
  [19] .strtab           STRTAB          00000000 04e2bc 001526 00      0   0  1
  [20] .shstrtab         STRTAB          00000000 04f7e2 0000c2 00      0   0  1
Key to Flags:
  W (write), A (alloc), X (execute), M (merge), S (strings), I (info),
  L (link order), O (extra OS processing required), G (group), T (TLS),
  C (compressed), x (unknown), o (OS specific), E (exclude),
```





https://www.coursera.org/lecture/jisuanji-xitong/w10-3-2-elftou-he-jie-tou-biao-2HMUB

https://refspecs.linuxfoundation.org/elf/elf.pdf