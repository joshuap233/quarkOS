可重入:

若一个[程序](https://zh.wikipedia.org/wiki/程序)或[子程序](https://zh.wikipedia.org/wiki/子程序)可以“在任意时刻被[中断](https://zh.wikipedia.org/wiki/中断)然后操作系统调度执行另外一段代码，这段代码又调用了该子程序不会出错”，则称其为**可重入**（reentrant或re-entrant）的。即当该子程序正在[运行时](https://zh.wikipedia.org/wiki/執行期)，执行线程可以再次进入并执行它，仍然获得符合设计时预期的结果。与多线程并发执行的[线程安全](https://zh.wikipedia.org/wiki/线程安全)不同，可重入强调对单个线程执行时重新进入同一个子程序仍然是安全的。





本文的所有的讨论都是基于 linux, x64 架构



<hr/>



最近在线程方面的资料，看到一堆概念：用户线程，内核线程，lwp 轻量级进程,协程...这些复杂的概念弄得我一头雾水，现在我来一点点地理清这些概念



这里所有讨论的线程都以 linux 为基础，不讨论 windows 的实现



linux 里所有带有线程的东西，本质都是进程，都有 task_struct 结构管理，统一叫做任务



## 内核线程

两种

一种是只在内核空间运行的线程，没有 strcut mm_struct 结构，像我在 QuarkOS 上实现的一样

另一种是用户线程，不过由内核调度，这也是 pthead 的实现



## lwp 轻量级进程 

也就是 linux 的第二种内核线程，由 clone 调用创建



## 用户线程

两种

1.在用户空间由用户线程包实现的线程

线程调度由用户线程包实现

优点：内核对这种用户线程一无所知，内核以进程为基本调度单位，调度不必陷入内核，可以在不支持线程的操作系统上实现可以使用 thread_yield 主动切换线程，可以定制自己的调度算法

问题：遇到阻塞调用时，整个进程都会阻塞，进程没有完全消耗完时间片

可能的解决：使用 select api 判断某个调用，比如 read 会阻塞，如果会阻塞则线程主动切换到其他线程，切换会时再次判断是否会阻塞

还一个问题时，必须等待一个线程主动释放 CPU，另一个线程才能运行，用户线程调度器无法利用时钟中断调度



2.在内核实现的线程，由内核调用这是 pthread 在linux 上的实现

下面是 pthread 片段代码:

```c
 
const int clone_flags = (CLONE_VM | CLONE_FS | CLONE_FILES | CLONE_SYSVSEM
			   | CLONE_SIGHAND | CLONE_THREAD
			   | CLONE_SETTLS | CLONE_PARENT_SETTID
			   | CLONE_CHILD_CLEARTID
			   | 0);

  TLS_DEFINE_INIT_TP (tp, pd);

#ifdef __NR_clone2
# define ARCH_CLONE __clone2
#else
# define ARCH_CLONE __clone
#endif
  if (__glibc_unlikely (ARCH_CLONE (&start_thread, STACK_VARIABLES_ARGS,
				    clone_flags, pd, &pd->tid, tp, &pd->tid)
			== -1))
    return errno;
```



以及 pthread_yield 的 man 手册:

```
       On Linux, this function is implemented as a call to sched_yield(2).
```



sched_yield 的 man 手册

```
     sched_yield()  causes  the  calling  thread to relinquish the CPU.  The
       thread is moved to the end of the queue for its static priority  and  a
       new thread gets to run.
```



在 linux 上 pthread_yield 由 sched_yield 实现, 而 sched_yield 会陷入内核,切换到当前线程组的可调用线程



在使用 clone 调用时,传入 CLONE_THREAD 参数,内核会将创建的线程与当前线分到同一组以方便调度,

值得注意的时 sched_yield 只能切换线程,也就是 lwp

使用 ps -eLF 可以看到 lwp 栏,也就是 线程 id

```
❯ ps -eLF
UID          PID    PPID     LWP  C NLWP    SZ   RSS PSR STIME TTY          TIME CMD
root           1       0       1  0    1 41990  7172   7 10:21 ?        00:00:02 /sbin/init splash
root           2       0       2  0    1     0     0   2 10:21 ?        00:00:00 [kthreadd]
root           3       2       3  0    1     0     0   0 10:21 ?        00:00:00 [rcu_gp]
root           4       2       4  0    1     0     0   0 10:21 ?        00:00:00 [rcu_par_gp]
root           6       2       6  0    1     0     0   0 10:21 ?        00:00:00 [kworker/0:0H-kblockd]
root           9       2       9  0    1     0     0   0 10:21 ?        00:00:00 [mm_percpu_wq]
pjs        87203   86752   87203  0   17  1135  1532   0 23:10 pts/0    00:00:00 /home/pjs/.cache/git...
pjs        87203   86752   87204  0   17  1135  1532   5 23:10 pts/0    00:00:00 /home/pjs/.ca..
pjs        87203   86752   87205  0   17  1135  1532   2 23:10 pts/0    00:00:00 /home/pjs/.cache/g..
pjs        87203   86752   87206  0   17  1135  1532   3 23:10 pts/0    00:00:00 /home/pjs/.cache/gi..
pjs        87203   86752   87207  0   17  1135  1532   4 23:10 pts/0    00:00:00 /home/pjs/.cach..
pjs        87203   86752   87208  0   17  1135  1532   4 23:10 pts/0    00:00:00 /home/pjs/..
pjs        87203   86752   87209  0   17  1135  1532   4 23:10 pts/0    00:00:00 /home/pjs/.cache/gi..

```



一些任务由相同的 pid ,ppid ,但 lwp 不同



## 协程 coroutine

就是用户线程的第一种,不过协程在内核没有线程信息而已....

看了下风云 实现的 coroutine, https://github.com/cloudwu/coroutine

使用 ucontext 实现,但是 ucontext 会陷入内核啊!!!!这和 pthread 有啥区别.......pthread 也有 yield 好吧...



### setjmp longjmp

setjmp /longjmp 可以在函数间直接跳转

```c
static jmp_buf jmpBuf;
static int globval;

static void f1(int i, int j, int k, int l);

int main() {
    int autoval = 1;
    register int regival = 2;
    volatile int volaval = 3;
    static int statval = 4;

    if (setjmp(jmpBuf) != 0) {
        printf("after longjump:\n");
        printf("globval = %d, autoval= %d, regival = %d,"
               " volaval = %d, statval = %d\n", globval, autoval, regival, volaval, statval);
        exit(0);
    }
    globval = 95;
    autoval = 96;
    regival = 97;
    volaval = 98;
    statval = 99;

    f1(autoval, regival, volaval, statval);
    exit(0);
}

static void f1(int i, int j, int k, int l) {
    printf("int f1():\n");
    printf("globval = %d, autoval= %d, regival = %d,"
           " volaval = %d, statval = %d\n", globval, i, j, k, l);
    longjmp(jmpBuf, 1);
}
```

与 fork 设计类似,直接调用 setjmp ,返回 0, 从 longjmp 返回时,返回  longjmp 传入的第二个参数

gcc 编译时开启优化参数，

```
❯ gcc main.c -O0
❯ ./a.out
int f1():
globval = 95, autoval= 96, regival = 97, volaval = 98, statval = 99
after longjump:
globval = 95, autoval= 96, regival = 2, volaval = 98, statval = 99
```



```
❯ gcc main.c -O1
❯ ./a.out
int f1():
globval = 95, autoval= 96, regival = 97, volaval = 98, statval = 99
after longjump:
globval = 95, autoval= 1, regival = 2, volaval = 98, statval = 99

```

 

gcc 开启优化参数与不开启优化参数编译运行的结果不同，为开启优化参数时，跳转前后，全局变量，自动变量（局部变量），寄存器变量，valitale 变量，静态变量的值中，只有 局部变量的值发生了改变，而开启优化参数后，寄存器变量的值也发生了变化，因此使用 setjmp 需要避免使用局部变量，或者弃用之前的局部变量





