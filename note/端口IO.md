

| 指令 | 作用            | 注释                                                         |
| ---- | --------------- | ------------------------------------------------------------ |
| IN   | Input from port | (1) `AL = port[imm];` (2) `AL = port[DX];` (3) `AX = port[imm];` (4) `AX = port[DX];` |
| OUT  | Output to port  | (1) `port[imm] = AL;` (2) `port[DX] = AL;` (3) `port[imm] = AX;` (4) `port[DX] = AX;` |


0x80 端口

https://www.intel.com/content/www/us/en/support/articles/000005500/boards-and-kits.html#port80h



端口读写的一个小窍门,读写连续两个端口时,比如 读写 端口 1,2 8字节, 

可以写成

```c
outb(port, LOBYTE(data));
outb(port, HIBYTE(data));
```



也可以写成

```c
outw(port, data);
```



见: https://forum.osdev.org/viewtopic.php?f=13&t=31706