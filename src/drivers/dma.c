//
// Created by pjs on 2021/4/14.
//
// ide dma


//For the primary channel, ALTSTATUS/CONTROL port is BAR1 + 2.
//For the secondary channel, ALTSTATUS/CONTROL port is BAR3 + 2.

//Data Register: BAR0 + 0; // Read-Write
//Error Register: BAR0 + 1; // Read Only
//Features Register: BAR0 + 1; // Write Only
//SECCOUNT0: BAR0 + 2; // Read-Write
//LBA0: BAR0 + 3; // Read-Write
//LBA1: BAR0 + 4; // Read-Write
//LBA2: BAR0 + 5; // Read-Write
//HDDEVSEL: BAR0 + 6; // Read-Write, used to select a drive in the channel.
//Command Register: BAR0 + 7; // Write Only.
//Status Register: BAR0 + 7; // Read Only.
//Alternate Status Register: BAR1 + 2; // Read Only.
//Control Register: BAR1 + 2; // Write Only.
//DEVADDRESS: BAR1 + 3; // I don't know what is the benefit from this register.
