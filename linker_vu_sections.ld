/* linker_vu_sections.ld - include into your main linker script */
SECTIONS
{
    /* map VU microcode into a known ELF section; address can be relocated at load time */
    .vutext 0x00200000 : ALIGN(0x40)
    {
        KEEP(*(.vutext))
    } > RAM
}