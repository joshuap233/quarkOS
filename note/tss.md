利用分页,每个进程都有不同的虚拟内存,那么ldt还有什么用???



CPU 特权级(x86)

To carry out privilege-level checks between code segments and data segments, the processor recognizes the following three types of privilege levels:

- **Current privilege level (CPL)**: 

  The CPL is the privilege level of the currently executing program or task. **It is stored in bits 0 and 1 of the CS and SS segment registers**. Normally, the CPL is equal to the privilege level of the code segment from which instructions are being fetched. The processor changes the CPL when program control is transferred to a code segment with a different privilege level. The CPL is treated slightly differently
  when accessing **conforming code segments**. Conforming code segments can be accessed from any privilege level that is equal to or numerically greater (less privileged) than the DPL of the conforming code segment. Also, the CPL is not changed when the processor accesses a conforming code segment that has a different privilege level than the CPL.

- **Descriptor privilege level (DPL)** :
  
  The DPL is the privilege level of a segment or gate. **It is stored in the DPL field of the segment or gate descriptor for the segment or gate**. When the currently executing code segment attempts to access a segment or gate, the DPL of the segment or gate is compared to the CPL and RPL of the segment or gate selector (as described later in this section). The DPL is interpreted differently, depending on the type of segment or gate being accessed:
  
  - **Data segment** — The DPL indicates the numerically highest privilege level that a program or task can have to be allowed to access the segment. For example, if the DPL of a data segment is 1, only programs running at a CPL of 0 or 1 can access the segment.
  - **Nonconforming code segment (without using a call gate)** — The DPL indicates the privilege level that a program or task must be at to access the segment. For example, if the DPL of a  nonconforming code segment is 0, only programs running at a CPL of 0 can access the segment.
  - **Call gate** — The DPL indicates the numerically highest privilege level that the currently executing program or task can be at and still be able to access the call gate. (This is the same access rule as for a data segment.)
  - **Conforming code segment and nonconforming code segment accessed through a call gate** — The DPL indicates the numerically lowest privilege level that a program or task can have to be allowed to access the segment. For example, if the DPL of a conforming code segment is 2, programs running at a CPL of 0 or 1 cannot access the segment.
  -  **TSS** — The DPL indicates the numerically highest privilege level that the currently executing program or task can be at and still be able to access the TSS. (This is the same access rule as for a data segment.)
  
- **Requested privilege level (RPL)** — The RPL is an override privilege level that is assigned to segment
  selectors. **It is stored in bits 0 and 1 of the segment selector.** The processor checks the RPL along with the CPL to determine if access to a segment is allowed. Even if the program or task requesting access to a segment has sufficient privilege to access the segment, access is denied if the RPL is not of sufficient privilege level. That is, if the RPL of a segment selector is numerically greater than the CPL, the RPL overrides the CPL, and vice versa. The RPL can be used to insure that privileged code does not access a segment on behalf of an application program unless the program itself has access privileges for that segment. See Section 5.10.4, “Checking Caller Access Privileges (ARPL Instruction),” for a detailed description of the purpose and typical use of the RPL.

注意段寄存器与段选择子的区别:

- A segment selector is a 16-bit identifier for a segment

  也就是说段寄存器存储的数值叫段选择子



=============================tss========================

Besides code, data, and stack segments that make up the execution environment of a program or procedure, the architecture defines two system segments: the task-state segment (TSS) and the LDT



The IA-32 architecture provides a mechanism for saving the state of a task, for dispatching tasks for execution, and for switching from one task to another. When operating in protected mode, all processor execution takes place from within a task. Even simple systems must define at least one task. More complex systems can use the processor’s task management facilities to support multitasking applications



A task is made up of two parts: a task execution space and a task-state segment (TSS)



Besides code, data, and stack segments that make up the execution environment of a program or procedure, the architecture defines two system segments: the task-state segment (TSS) and the LDT. The GDT is not considered a segment because it is not accessed by means of a segment selector and segment descriptor. TSSs and LDTs have segment descriptors defined for them.

gdt 与 idt不被认为是段,但 ldt 与 tss 是, 因为ldt 与 tss 有对应的描述符和选择子

The architecture also defines a set of special descriptors called gates (call gates, interrupt gates, trap gates, and task gates).

A CALL to a call gate can provide access to a procedure in a code segment that is at the same or a numerically lower privilege level (more privileged) than the current code segmen.



If an operating system or executive uses the processor’s privilege-level protection mechanism, the task execution space also provides a separate stack for each privilege leve



![2021-02-16 11-00-11 的屏幕截图](image/2021-02-16%2011-00-11%20%E7%9A%84%E5%B1%8F%E5%B9%95%E6%88%AA%E5%9B%BE.png)

The GDT is not a segment itself; instead, it is a data structure in linear address space. The base linear address and limit of the GDT must be loaded into the GDTR register

gdt 本身不是一个段,而是一个在线性地址空间的数据结构,cpu 利用 gdtr 的基地址与段选择子的偏移转换为线性地址,找到 gdt

TSS descriptors may only be placed in the GDT; they cannot be placed in an LDT or the IDT.



tss 结构:

![2021-02-14 19-12-56 的屏幕截图](image/2021-02-14%2019-12-56%20%E7%9A%84%E5%B1%8F%E5%B9%95%E6%88%AA%E5%9B%BE.png)



The TSS, like all other segments, is defined by a segment descriptor.

TSS descriptors may only be placed in the GDT; they cannot be placed in an LDT or the IDT.

![2021-02-14 19-17-16 的屏幕截图](image/2021-02-14%2019-17-16%20%E7%9A%84%E5%B1%8F%E5%B9%95%E6%88%AA%E5%9B%BE.png)

base 指的是 tss 首地址偏移, limit  >= tss 大小ty



The same alignment should be used when storing the IDTR register using the SIDT instruction. When storing the LDTR or task register (using the SLDT or STR instruction, respectively), the pseudo-descriptor should be located at a doubleword address (that is, address MOD 4 is equal to 0).

The busy flag (B) in the type field indicates whether the task is busy. A busy task is currently running or suspended. A type field with a value of 1001B indicates an inactive task; a value of 1011B indicates a busy task. Tasks are not recursive. The processor uses the busy flag to detect an attempt to call a task whose execution has been inter-rupted. To insure that there is only one busy flag is associated with a task, each TSS should have only one TSS descriptor that points to it.



When the G flag is 0 in a TSS descriptor for a 32- bit TSS, the limit field must have a value equal to or greater than 67H, one byte less than the minimum size of a TSS. Attempting to switch to a task whose TSS descriptor has a limit less than 67H generates an invalid-TSS excep-tion (#TS). A larger limit is required if an I/O permission bit map is included or if the operating system stores addi-tional data. The processor does not check for a limit greater than 67H on a task switch; however, it does check when accessing the I/O permission bit map or interrupt redirection bit map



task register:

![2021-02-16 12-06-56 的屏幕截图](image/2021-02-16%2012-06-56%20%E7%9A%84%E5%B1%8F%E5%B9%95%E6%88%AA%E5%9B%BE.png)

The task register holds the 16-bit segment selector and the entire segment descriptor (32-bit base address , 16-bit segment limit, and descriptor attributes) for the TSS of the current task (see Figure 2-6).This information is copied from the TSS descriptor in the GDT for the current task.

ts 寄存器只有 0-15 是可见部分,不可见部分用于缓存描述符



![2021-02-17 21-34-40 的屏幕截图](../../../Pictures/2021-02-17%2021-34-40%20%E7%9A%84%E5%B1%8F%E5%B9%95%E6%88%AA%E5%9B%BE.png)





**Task-Gate Descriptor:**

A task-gate descriptor provides an indirect, protected reference to a task. It can be placed in the
GDT, an LDT, or the IDT.The TSS segment selector field in a task-gate descriptor points to a TSS descriptor in the GDT. 

Note that when a task gate is used, the DPL of the destination TSS descriptor is not used.

![2021-02-17 21-53-32 的屏幕截图](image/2021-02-17%2021-53-32%20%E7%9A%84%E5%B1%8F%E5%B9%95%E6%88%AA%E5%9B%BE.png)

A program or procedure that does not have sufficient privilege to access the TSS descriptor for a task in the GDT (which usually has a DPL of 0) may be allowed access to the task through a task gate with a higher DPL. Task gates give the operating system greater latitude for limiting access to specific tasks.

task gate 的dpl 会覆盖描述符的dpl



Task gates may also reside in the IDT, which allows interrupts and exceptions to be handled by handler tasks. When an interrupt or exception vector points to a task gate, the processor switches to the specified task.



In most systems, the DPLs of TSS descriptors are set to values less than 3, so that only privileged software can perform task switching. However, in multitasking applications, DPLs for some TSS descriptors may be set to 3 to allow task switching at the application (or user) privilege level.





**TASK SWITCHING**

The processor transfers execution to another task in one of four cases:

- The current program, task, or procedure executes a JMP or CALL instruction to a TSS descriptor in the GDT.
- The current program, task, or procedure executes a JMP or CALL instruction to a task-gate descriptor in the GDT or the current LDT.
- An interrupt or exception vector points to a task-gate descriptor in the IDT.
- The current task executes an IRET when the NT flag in the EFLAGS register is set

见手册7.3



任务链接:



![2021-02-18 13-09-16 的屏幕截图](image/2021-02-18%2013-09-16%20%E7%9A%84%E5%B1%8F%E5%B9%95%E6%88%AA%E5%9B%BE.png)



When a CALL instruction, an interrupt, or an exception causes a task switch: 产生嵌套任务(NT=1)



When a JMP instruction causes a task switch, the new task is not nested(EFLAGS.NT=0)



the operating system should initialize the previous task link field in every TSS that it creates to 0.



busy flag:

如果任务链构成一个环状,那么 iret 返回或call到新 task 时,可能会陷入任务的无限递归, busy flag 用于解决这个问题

The processor prevents recursive task switching by preventing a task from switching to itself or to any task in a nested chain of tasks



In a uniprocessor system, in situations where it is necessary to remove a task from a chain of linked tasks, use the following procedure to remove the task:

1. Disable interrupts.
2. Change the previous task link field in the TSS of the pre-empting task (the task that suspended the task to be removed). It is assumed that the pre-empting task is the next task (newer task) in the chain from the task to be removed. Change the previous task link field to point to the TSS of the next oldest task in the chain or to an even older task in the chain
3. Clear the busy (B) flag in the TSS segment descriptor for the task being removed from the chain. If more than one task is being removed from the chain, the busy flag for each task being remove must be cleared.



task address space

With either method of mapping task linear address spaces, the TSSs for all tasks must lie in a shared area of the physical space, which is accessible to all tasks.

tss 与 gdt 映射需要所有任务共享,所以我决定将内核映射到 3-4G,所有任务共享内核而不是创建一个新共享段



任务共享内存: 见手册 7.5.2

- Through the segment descriptors in the GDT
- Through a shared LDT
- Through segment descriptors in distinct LDTs that are mapped to common addresses in linear
  address space

====================================================================================

x86 任务切换:

Here, the segment selector for the TSS of the new task is given in the CALL or JMP instruction. In switching tasks, the processor performs the following actions:
1. Stores the state of the current task in the current TSS.
2. Loads the task register with the segment selector for the new task.
3. Accesses the new TSS through a segment descriptor in the GDT.
4. Loads the state of the new task from the new TSS into the general-purpose registers, the segment registers, the LDTR, control register CR3 (base address of the paging-structure hierarchy), the EFLAGS register, and the EIP register.
5. Begins execution of the new task. A task can also be accessed through a task gate. A task gate is similar to



Gate descriptors in the IDT can be interrupt, trap, or task gate descriptors

To access an interrupt or exception handler, the processor first receives an interrupt vector from internal hardware, an external interrupt controller, or from software by means of an INT, INTO, INT 3, or BOUND instruction.



The LDT is located in a system segment of the LDT type. The GDT must contain a segment descriptor for the LDT segment. If the system supports multiple LDTs, each must have a separate segment selector and segment descriptor in the GDT.

An LDT is accessed with its segment selector. To eliminate address translations when accessing the LDT, the segment selector, base linear address, limit, and access rights of the LDT are stored in the LDTR register

![2021-02-16 12-06-56 的屏幕截图](image/2021-02-16%2012-06-56%20%E7%9A%84%E5%B1%8F%E5%B9%95%E6%88%AA%E5%9B%BE.png)



使用 x86 进行任务上下文切换叫硬件切换,这么做效率很低, Linux 采用软件切换的形式, Linux 实际只用了  TSS 中 的 SS0 和 esp0(切换到不用的特权级需要不同的栈), ss0,esp0 用于保存切换到 0 特权级时的栈位置 , 这叫软件上下文切换,也就是一个 CPU 一个 tss, 而不是一个任务一个 tss







![2021-02-16 11-48-11 的屏幕截图](image/2021-02-16%2011-48-11%20%E7%9A%84%E5%B1%8F%E5%B9%95%E6%88%AA%E5%9B%BE.png)



## 3.4.2 Segment Selectors
A segment selector is a 16-bit identifier for a segment (see Figure 3-6). It does not point directly to the segment, but instead points to the segment descriptor that defines the segment. A segment selector contains the following items:

**Index:**
(Bits 3 through 15) — Selects one of 8192 descriptors in the GDT or LDT. The processor multiplies the index value by 8 (the number of bytes in a segment descriptor) and adds the result to the base address of the GDT or LDT (from the GDTR or LDTR register, respectively).



**TI (table indicator) flag**
(Bit 2) — Specifies the descriptor table to use: clearing this flag selects the GDT; setting this flag selects the current LDT.



**Requested Privilege Level (RPL)**
(Bits 0 and 1) — Specifies the privilege level of the selector. The privilege level can range from 0 to 3, with 0 being the most privileged level. See Section 5.5, “Privilege Levels”, for a description of the relationship of the RPL to the CPL of the executing program (or task) and the descriptor privilege level (DPL) of the descriptor the segment selector points to.

![2021-02-16 14-02-59 的屏幕截图](image/2021-02-16%2014-02-59%20%E7%9A%84%E5%B1%8F%E5%B9%95%E6%88%AA%E5%9B%BE.png)



代码段描述符有一个 c 位(conforming), 如果设置了该位,则这个代码段为 conforming segment(依从代码段)





特权级检查:

- **PRIVILEGE LEVEL CHECKING WHEN ACCESSING DATA SEGMENTS**

  - The processor loads the segment selector into the segment register if the DPL is numerically greater than or equal to both the CPL and the RPL. Otherwise, a general-protection fault is generated and the segment register is not loaded.

- **PRIVILEGE LEVEL CHECKING WHEN LOADING THE SS REGISTER**

  - the CPL, the RPL of the stack-segment selector, and the DPL of the stack-segment descriptor must be the same.

- **PRIVILEGE LEVEL CHECKING WHEN TRANSFERRING PROGRAM CONTROL BETWEEN CODE SEGMENTS**

  - Program control transfers are carried out with the JMP, CALL, RET, SYSENTER, SYSEXIT, SYSCALL, SYSRET, INT n, and IRET instructions, as well as by the exception and interrupt mechanisms.

  

  **5.8.1 Direct Calls or Jumps to Code Segments**
  
  - A JMP or CALL instruction can reference another code segment in any of four ways:
  - The target operand contains the segment selector for the target code segment.
  -  The target operand points to a TSS, which contains the segment selector for the target code segment.
    - The target operand points to a call-gate descriptor, which contains the segment selector for the target code segment.
  - The target operand points to a task gate, which points to a TSS, which in turn contains the segment selector for the target code segment.
    
  - When transferring program control to another code segment without going through a call gate, the processor examines four kinds of privilege level and type information:
    - CPL
    - RPL
    - DPL
    - C flag

![2021-02-16 19-29-39 的屏幕截图](image/2021-02-16%2019-29-39%20%E7%9A%84%E5%B1%8F%E5%B9%95%E6%88%AA%E5%9B%BE.png)

 **5.8.1.1 Accessing Nonconforming Code Segments(c flag置 0)**

When accessing nonconforming code segments, the CPL of the calling procedure must be equal to the DPL of the destination code segment;



**5.8.1.2 Accessing Conforming Code Segments**

When accessing conforming code segments, the CPL of the calling procedure may be numerically equal to or greater than (less privileged) the DPL of the destination code segment(The segment selector RPL for the destination code segment is not checked if the segment is a conforming code segment)





**5.8.2 Gate Descriptors**

To provide controlled access to code segments with different privilege levels, the processor provides special set of
descriptors called gate descriptors. There are four kinds of gate descriptors:

- Call gates
- Trap gates
- Interrupt gates
- Task gates

Task gates are used for task switching and are discussed in Chapter 7, “Task Management”. **Trap and interrupt gates are special kinds of call gates used for calling exception and interrupt handlers.** The are described in Chapter6, “Interrupt and Exception Handling.” This chapter is concerned only with call gates.



**call gate:**

用于访问优先级更高的代码()

可以用于实现系统调用

Call gates facilitate controlled transfers of program control between different privilege levels. They are typically used only in operating systems or executives that use the privilege-level protection mechanism. Call gates are also useful for transferring program control between 16-bit and 32-bit code segments, as described in Section 21.4, “Transferring Control Among Mixed-Size Code Segments.”

![2021-02-16 20-04-08 的屏幕截图](image/2021-02-16%2020-04-08%20%E7%9A%84%E5%B1%8F%E5%B9%95%E6%88%AA%E5%9B%BE-1613477085261.png)



param count 指定需要传递的参数个数

![2021-02-16 20-22-50 的屏幕截图](image/2021-02-16%2020-22-50%20%E7%9A%84%E5%B1%8F%E5%B9%95%E6%88%AA%E5%9B%BE.png)

以上比较为数值上的比较

**If a call is made to a more privileged (numerically lower privilege level) nonconforming destination code segment, the CPL is lowered to the DPL of the destination code segment and a stack switch occurs (see Section 5.8.5, “Stack Switching”). If a call or jump is made to a more privileged conforming destination code segment, the CPL is not changed and no stack switch occurs.**

![2021-02-16 20-38-13 的屏幕截图](image/2021-02-16%2020-38-13%20%E7%9A%84%E5%B1%8F%E5%B9%95%E6%88%AA%E5%9B%BE.png)



Call gates allow a single code segment to have procedures that can be accessed at different privilege levels. For example, an operating system located in a code segment may have some services which are intended to be used by both the operating system and application software (such as procedures for handling character I/O). Call gates for these procedures can be set up that allow access at all privilege levels (0 through 3). More privileged call gates (with DPLs of 0 or 1) can then be set up for other operating system services that are intended to be used only by the operating system (such as procedures that initialize device drivers)





SYSENTER and SYSEXIT (64 位模式叫 SYCALL and SYSRET):

可以用于实现系统调用



早期的 linux 为了兼容性使用中断实现系统调用,尽管 call gate 与 SYSENTER and SYSEXIT 指令更快



TODO: call gate /ldtr ldt tss



task gate:

A task can also be accessed through a task gate. A task gate is similar to a call gate, except that it provides access (through a segment selector) to a TSS rather than a code segment.

Gate descriptors in the IDT can be interrupt, trap, or task gate descriptors

暂时忽略