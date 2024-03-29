

## GDT

段描述符存储和段有关的信息，一个段描述符占 8 字节

最主要的描述符表是全局描述符表（GTD），为整个软硬件服务（即使使用平坦模型，依然需要定义GDT）

**GDTR** :存储全局表述符表地址的寄存器（48位），结构如下：



| 16-47位       | 0-15    |
| ------------- | ------- |
| GDT线性基地址 | GDT边界 |

0-15 bit 为 gdt 大小(byte) -1

即可以容纳 2**16/8 个描述符





描述符结构：



![img](image/GDT_Entry.png)





低 32 位：

| 31-16位          | 15-0           |
| ---------------- | -------------- |
| 段基地址（0-15） | 段界限（15-0） |

高 32 位：

| 31-24位         | 23   | 22   | 21   | 20   | 19-16           | 15   | 14-13 | 12   | 11-8 | 7-0             |
| --------------- | ---- | ---- | ---- | ---- | --------------- | ---- | ----- | ---- | ---- | --------------- |
| 段基址（31-24） | G    | D/B  | L    | AVL  | 段界限（19-16） | P    | DPL   | S    | TYPE | 段基址（23-16） |

intel 为了兼容80826，把段基址和段界限都拆成了两部分

- 32 位基址
- 20 位界限（最大可寻址单元字节数或页数，如果以页（4K）为单位，且界限值为 0xfffff，则恰好可寻址 4GB）



标志：

- AVL保留，可以给软件用，置 0 即可

- L 是 64位处理器的标志，置 0 即可
- D/B 用于指示操作数大小，置 1（32位操作数）
- G粒度位，即界限粒度为页还是字节，置 1（以页为单位）



Access Byte：

- P 段存在位，如果段在内存置 1，由于内存紧张，可能只建立了描述符，而内存中没有实际的段，或者段被交换到磁盘中，此时置 0
- DPL 段描述符特权级（0，1，2，3，4，最高为0），0给 内核用，3 给用户，1-2给驱动
- S 描述符类型，系统段为 0（如TSS段），代码或数据段为 1
- TYPE：分别为（低址到高址）：
  - A  访问位，当段最近被访问时，处理器自动置 1，操作系统可以定期监视并置 0
  - RW 指示读写权限
    - 代码段置 1 表示可读（无论值为 1还是0，代码段都不可写）
    - 数据段置 1 表示可写
  - EC  表示扩展方向
    - 数据段: 1 为向上（高地址）扩展,0 为向下扩展
    - 代码段: 1 表示允许从低特权级的程序转移到该段执行
  - X  置 1 为可执行
    - 代码段置 1
    - 数据段置 0



## 分页

采用二级页表    

结构: 

- 页目录（PD），4K
  - 存储 1024 个页表的物理地址（页表项）
  -  2**10 个页表项
  - 每个页表项位 4 字节（32比特）
- 页表(PT)，4K
  - 存储页的物理地址
- 内存页，4K
- cr3 寄存器，存放当前任务页目录的物理地址，32位



已知 PDTR、32位虚拟地址（高10位为页目录索引，中间10位为页表索引，第12位为页内偏移）



cr3 (PDBR)结构:

![2021-02-02 15-31-25 的屏幕截图](image/2021-02-02%2015-31-25%20%E7%9A%84%E5%B1%8F%E5%B9%95%E6%88%AA%E5%9B%BE.png)

- 0-2置 0
- 3: pwd
- 4: PCD
- 5-11 置 0
- 12 -31 页目录物理地址
- 32-63 只用于 intel64,否则忽略







页目录项与页表项结构结构:

![2021-02-02 11-31-47 的屏幕截图](image/2021-02-02%2011-31-47%20%E7%9A%84%E5%B1%8F%E5%B9%95%E6%88%AA%E5%9B%BE.png)



页目录,页表,页面都需要 页对齐(4K),因此低 12 位用于记录属性,高 20 位位地址

页目录项:

- 如果 bit 7 (S 位)被置 1 ,则采用 4MB 页表(即下面的 PDE: 4MB page), 否则采用 4KB 页表(即下面的 PDE page table), 开启 4MB 页需要 cr4 PSE 位被设置

- A: 访问位,与 GDT 的访问位类似,如果页面被读取则置 1,不同的是,CPU 不会自动清除该位,因此需要操作系统来做
- PCD:  置 1 ,则页面不会被缓存
- PWT:  被置 1 时使用 "write-through", 否则使用 "write-back",置 0
  - write-through: 
  - write-back
- U/S: 如果置1,则所有特权级都能访问(允许 user-mode acces)这个页目录项对应的页表, 置 0,  特权级 0,1,2 可以访问(允许 supervisor-mode access)
  - 如果置 0,页目录/页表对应的页地址属于 supervisor-mode 地址, 否则属于 user-mode 地址 
- R/W: 置0 ,页面**可能**不允许读(见访问控制)
- bit 0: (P 位): 存在位,页面存在于物理内存置 1, 置 0 时, CPU 会忽略其他所有位



页表项:

- G:  置1时,当 cr3 被重置时, 不会清空 TLB 中该页缓存, CR4 必须设置相应的位才能开启此功能
- D: 脏位,如果置 1 ,则表示这个页被写过, CPU 设置该位,但不会自动清除该位
- PAT(page attribute table) 太麻烦,而且处理器不一定支持,直接置 0



清空 TLB:

- invlpg 指令清除部分条目
- 重载 cr3





访问控制:

太复杂了:见intel 开发手册卷3 4.6节

Every access to a linear address is either a supervisor-mode access or a user-mode access.

访问分为:  supervisor-mode access 与 user-mode access

对 GDT,LDT,IDT,TSS, 的访问属于隐式 supervisor-mode 访问,而其他 <3 的特权级访问属于显示 supervisor-mode 访问, 特权级=3 的访问属于 user-mode 访问

对于 supervisor-mode accesses:

- 可以从 supervisor-mode 地址读取数据
- 



## 分段

即便开启分页,即便使用平坦模型,也无法避开分段基址

x86 cpu 的段寄存器叫做段选择器,存储的地址叫做段选择子

段选择子的组成:

| 15-3位     | 2位  | 1-0位 |
| ---------- | ---- | ----- |
| 描述符索引 | TT   | RPL   |

寻址方式:

描述符表基址 + 使用描述符索引 * 8 查找描述符,然后将描述符存入高速缓存

TT 位: 置 0 时,描述符在 GDT 中,置 1 时,描述符在LDT 中

RPL位: 指定当前选择子的程序的特权级(0-3)


<hr/>
关于 linker.ld 4K 对齐:

```text
> i686-elf-readelf -a build/quarkOS.bin
Section Headers:
[Nr] Name              Type            Addr     Off    Size   ES Flg Lk Inf Al
[ 0]                   NULL            00000000 000000 000000 00      0   0  0
[ 1] .text             PROGBITS        00100000 001000 00055a 00  AX  0   0 4096
[ 2] .rodata           PROGBITS        00101000 002000 00001a 00   A  0   0 4096
[ 3] .rodata.str1.1    PROGBITS        0010101a 00201a 000021 01 AMS  0   0  1
[ 4] .bss              NOBITS          00102000 003000 004010 00  WA  0   0 4096
[ 5] .comment          PROGBITS        00000000 00203b 000012 01  MS  0   0  1
[ 6] .symtab           SYMTAB          00000000 002050 000250 10      7  21  4
[ 7] .strtab           STRTAB          00000000 0022a0 000181 00      0   0  1
[ 8] .shstrtab         STRTAB          00000000 002421 000046 00      0   0  1
```

addr 与 off 都是 16 进制,

4K 对齐指的是 off 而不是 addr





缓存:

段寄存器包括描述符告诉缓存器,缓存段描述符

TLB 缓存页表