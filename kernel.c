typedef unsigned char uint8_t;
typedef unsigned int uint32_t;
typedef uint32_t size_t;

extern char __bss[];
extern char __bss_end[];
extern char __stack_top[];

void *memset(void *buf, char c, size_t n) {
    uint8_t *ptr = (uint8_t *)buf;
    while (n--)
        *ptr++ = c;
    return buf;
}

void kernel_main(void) {
    /*
     * BSS is a section that contains uninitialized global and static variables
     *
     * In C/C++, these are variables declared but not explicitly initialized,
     * like:
     *
     * int global_array[1000];        // Goes to BSS
     * static char buffer[4096];      // Goes to BSS
     * int initialized_var = 42;      // Goes to .data, NOT BSS
     *
     * But C standard guarantees that uninitialized global/static variables
     * start as zero, so boot code should zero this memory range before calling
     * main()
     *
     * the .bss section is first initialized to zero using the memset function.
     * Although some bootloaders may recognize and zero-clear the .bss section,
     * but we initialize it manually just in case.
     */
    memset(__bss, 0, (size_t)__bss_end - (size_t)__bss);

    for (;;);
}

/* __attribute__((section(".text.boot")))
 *
 * Purpose: This attribute tells the compiler to place the function in a specific
 * section named .text.boot instead of the default .text section.
 *
 * Hit works:
 *
 * Normally, all functions go into the .text section of the object file
 * This attribute overrides that default behavior
 * The function's machine code will be placed in a custom section called .text.boot
 * The linker script can then specifically control where this section gets placed in memory
 *
 * Why use it:
 *
 * - Boot code placement: You want your boot/startup code to be at a specific memory
 *   location (usually the very beginning of your program)
 * - Memory layout control: Critical for bare-metal programming where you need precise
 *   control over where code resides
 * - Linker script coordination: Works with your linker script's
 *   KEEP(*(.text.boot)) directive
 */
__attribute__((section(".text.boot")))
__attribute__((naked))
void boot(void) {
    __asm__ __volatile__(
        "mv sp, %[stack_top]\n"
        "j kernel_main\n"
        :
        : [stack_top] "r" (__stack_top)
    );
}
