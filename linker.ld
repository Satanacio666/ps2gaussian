/* SPLATSTORM X - Complete PS2 Memory Layout */
/* PS2 EE memory map with all required symbols */

ENTRY(_start)

MEMORY {
    ram : ORIGIN = 0x00100000, LENGTH = 30M
    vu0 : ORIGIN = 0x11000000, LENGTH = 4K  
    vu1 : ORIGIN = 0x11004000, LENGTH = 16K
}

SECTIONS {
    .text : { 
        *(.text)
        *(.init)
        *(.fini)
    } > ram
    
    .rodata : { 
        *(.rodata)
        *(.rodata.*)
    } > ram
    
    .data : { 
        *(.data)
        *(.sdata)
    } > ram
    
    .ctors : {
        *(.ctors)
    } > ram
    
    .dtors : {
        *(.dtors)
    } > ram
    
    .jcr : {
        *(.jcr)
    } > ram
    
    .bss : { 
        _fbss = .;
        *(.bss)
        *(.sbss)
        _end = .;
    } > ram
    
    .vudata ALIGN(16) : {
        KEEP(*(.vutext))           /* All dvp-as outputs */
        KEEP(*(.vudata))           /* Any explicit VU data */
        KEEP(*(.vudata.*))         /* Numbered VU data sections */
    } > ram
    
    .vubss : {
        *(.vubss)
    } > ram
    
    /* Stack and heap setup */
    _stack_size = 0x100000;  /* 1MB stack */
    _stack = 0x02000000 - _stack_size;
    _heap_size = 0x100000;   /* 1MB heap */
    _gp = ALIGN(16);
}