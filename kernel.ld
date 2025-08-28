/*
 * a linker script is a file which defines the memory layout of executable
 * files. Based on the layout, the linker assigns memory addresses to functions
 * variables
 */

/* defines the entry point function of the program */
ENTRY(boot)

/* Begins the SECTIONS block, where you lay out output sections and control the
 * memory layout.
 */
SECTIONS {
    /* the . symbol (location counter) represents the current address.
     * it automatically increments as data is placed
     *
     * sets the location counter to 0x80200000. This means the first output
     * section that follows will start there. On bare-metal, this becomes
     * the link (virtual) address where your image expects to run.
     */
    . = 0x80200000;

    /* this section contains the code of the program */
    .text : {
        /*
         * 1.Finds all sections named .text.boot from all input files
         * 2.Places them at the beginning of the current output section (.text)
         * 3.Ensures that even with garbage collection enabled, this startup
         *   code won't be removed (KEEP)
         * 4.Since it's the first line in the .text section, the startup code will
         *   be placed at the very beginning of the program (address 0x80200000)
         */
        KEEP(*(.text.boot));
        /* Includes all normal code sections from all input files: both exact
         * .text and any subsections like .text.foo. The wildcard * means
         * from all files.
         */
        *(.text .text.*);
    }

    /* this section contains constant data that is read-only */
    /* Defines the output section .rodata, and applies ALIGN(4) to
     * its start (ensures it begins at a 4-byte bounda
     */
    .rodata : ALIGN(4) {
        /* Pulls in all read-only data (e.g., literal strings, const tables)
         * from any input .rodata and .rodata.* sections.
         */
        *(.rodata .rodata.*);
    }

    /* this section contains read/write data */
    .data : ALIGN(4) {
        /* Includes all initialized writable data from input .data and
         * .data.* sections.
         */
        *(.data .data.*);
    }

    /* Block started by sysmbol
     *
     * ths section contains read/write data with an initial value of zero.
     * BSS is typically of type NOBITS (occupies memory at runtime but 
     * not file space)
     */
    .bss : ALIGN(4) {
        /* Defines a global symbol __bss at the current location (start of
         * the BSS region). Useful to zero BSS in your startup code.
         */
        __bss = .;
        /* Includes all uninitialized data from input sections named .bss,
         * .bss., .sbss, and .sbss. (the small-BSS variants are used by
         * some ABIs/compilers).
         */
        *(.bss .bss.* .sbss .sbss.*);
        /* Defines a symbol at the end of whatever was just placed in .bss.
         * Together, __bss..__bss_end bounds let you clear BSS:
         * memset(__bss, 0, __bss_end - __bss).
         */
        __bss_end = .;
    }

    /* Realigns the location counter to a 4-byte boundary (useful before
     * defining symbols or carving out buffers).
     */
    . = ALIGN(4);
    /* Advances the location counter by 128 KiB, effectively reserving 128 KiB
     * of address space after BSS. No data is placed, but the address is moved
     * forward. This is often used to reserve stack space or a scratch area.
     */
    . += 128 * 1024; /* 128 KB */
    /* Defines the symbol __stack_top at the current location (which is “end of
     * BSS plus 128 KiB”). Startup code can set the stack pointer to __stack_top.
     */
    __stack_top = .;
}
