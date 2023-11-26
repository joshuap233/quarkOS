APIC ("Advanced Programmable Interrupt Controller") is the updated Intel standard for the older [PIC](https://wiki.osdev.org/PIC).

可以当成 PIC 的新版

见 intel 手册 第 10 章



Its primary function is to receive external interrupt events from the system and its associated I/O devices and relay them to the local APIC as interrupt messages. In MP systems, the I/O APIC also provides a mechanism for distributing external interrupts to the local APICs of selected processors or groups of processors on the system bus.



Each local APIC consists of a set of APIC registers (see Table 10-1) and associated hardware that control the
delivery of interrupts to the processor core and the generation of IPI messages. The APIC registers are memory mapped and can be read and written to using the MOV instruction 