# quarkOS

一个玩具操作系统


[参考资料](./doc)

## 构建
1.  [构建 i686-elf-gcc](https://wiki.osdev.org/GCC_Cross-Compiler)
2. `./build.sh`


## 运行

`./run.sh`

由于没有实现安装器，第一次运行时会将生成的 disk.img 挂载到 /tmp/disk，
并将用户空间代码复制到 disk.img 的 /bin 目录， 因此需要 root 权限 (见 generate-disk.sh)。


## 调试

建议使用 Clion，可以直接导入我的 .run 下配置，或者直接使用 gdb：

1. 允许读取 local .gdbint：
```bash
echo "set auto-load local-gdbinit on"  >> ~/.gdbinit
echo "add-auto-load-safe-path xxx/.gdbinit"  >> ~/.gdbinit
# 将 xxx 替换为你的 quarkOS 项目路径
```

2. `./debug.sh`
   
3. 启动 gdb，连接 1234 端口。

4. 添加用户空间代码时，需要将生成的文件添加到 .gdbinit 用于调试。

## 设计

![1.png](doc/1.png)


## TIPS:

- spin_lock 这类基于原子交换的结构(锁)在单核下的实现为关闭中断。
- 启动会运行测试代码，打印测试信息，需要在 src/include/types.h 中去除 TEST 宏，重新构建。

## TODO:

- 内存回收，(部分模块实现的内存回收函数，但底层 buddy 并没有调用)

- 更完善的用户空间( 它需要一个shell 😝 ) 

- 统一的错误码

- 部分代码不是线程安全的(这部分的数据结构可能修改，暂时不加锁)
 
- 网络协议栈

- SMP
