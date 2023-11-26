alignas 的问题



```c
_Alignas(4096)
u8_t foo1[10];


int main() {
    printf("%p\n", &foo1);
    printf("%p", &foo1 + 4096);
    return 0;
}

```



```c
0x55ce46de4000
0x55ce46dee000
```

偏移的内存为 40960 而不是 4096