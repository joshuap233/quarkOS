https://wiki.osdev.org/PCI_IDE_Controller



https://wiki.osdev.org/PCI#Configuration_Space



https://0cch.com/2013/08/31/e4bdbfe794a8pci-ide-controllere8afbbe58699e7a1ace79b98-2/



https://zhuanlan.zhihu.com/p/26244141



PCI spec规定了PCI设备必须提供的单独地址空间：配置空间（configuration space）,前64个字节（其地址范围为0x00~0x3F）是所有PCI设备必须支持的（有不少简单的设备也仅支持这些），此外PCI/PCI-X还扩展了0x40~0xFF这段配置空间，在这段空间主要存放一些与MSI或者MSI-X中断机制和电源管理相关的Capability结构。



Systems must provide a mechanism that allows access to the PCI configuration space, as most CPUs do not have any such mechanism

