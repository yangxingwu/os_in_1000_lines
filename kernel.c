#include "kernel.h"
#include "common.h"

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

// Standard SBI return type: error and value.
// The function sets up fixed registers (a0-a7) and issues an ecall.
// On return, a0 and a1 are read back as the SBI result.
struct sbiret sbi_call(long arg0, long arg1, long arg2, long arg3, long arg4,
                       long arg5, long fid, long eid) {
    /* Force C variables into specific RISC-V argument registers using GCC's
     * "register variable with asm" extension. This ensures the correct ABI
     * layout.
     *
     * SBI (Supervisor Binary Interface) call convention (v0.2+ style):
     *
     *   a7: extension ID (EID)
     *   a6: function ID (FID)
     *   a0..a5: up to 6 argument registers
     *
     *   returns: a0 = error code, a1 = return value
     */
    register long a0 __asm__("a0") = arg0;
    register long a1 __asm__("a1") = arg1;
    register long a2 __asm__("a2") = arg2;
    register long a3 __asm__("a3") = arg3;
    register long a4 __asm__("a4") = arg4;
    register long a5 __asm__("a5") = arg5;
    register long a6 __asm__("a6") = fid;
    register long a7 __asm__("a7") = eid;

    /* ecall is the RISC-V instruction that triggers a synchronous trap to the
     * operating environment (a higher-privileged handler), typically used to
     * request services such as system calls or firmware functions.
     *
     * Purpose and typical use:
     *
     * 1.User mode → OS kernel: User programs execute ecall to request system
     *   services (file I/O, process control, etc.). The trap transfers control
     *   to the kernel’s trap/exception handler.
     * 2.Supervisor mode → Machine-mode firmware (SBI): An OS kernel running
     *   in S-mode can execute ecall to call into the SBI (Supervisor Binary
     *   Interface) implemented in M-mode firmware for operations like
     *   console I/O, timer, and IPI.
     *
     * Clobbers: "memory" to prevent the compiler from reordering memory
     * operations across the ECALL boundary, since the SBI may have side
     * effects.
     */
    __asm__ __volatile__(
        "ecall"
        : "=r"(a0), "=r"(a1)
        : "r"(a0), "r"(a1), "r"(a2), "r"(a3), "r"(a4), "r"(a5),
          "r"(a6), "r"(a7)
        : "memory"
    );

    // Package the SBI result into sbiret: error in a0, value in a1.
    return (struct sbiret){.error = a0, .value = a1};
}

// Output a single character to the console using the SBI legacy console putchar
// Convention (commonly used legacy extension):
// - EID = 1 (legacy console extension)
// - FID = 0 or 1 depending on the platform's legacy mapping; here it's 0 as used
//   in many minimal examples for "Console Putchar" under legacy SBI.
// - arg0 = character to print
void putchar(char ch) {
    sbi_call(ch, 0, 0, 0, 0, 0, 0, 1 /* Console Putchar */);
}

void kernel_main(void) {
    const char *s = "\n\nHello World!\n";

    for (int i = 0; s[i] != '\0'; i++) {
        putchar(s[i]);
    }

    printk("\n\nHello %s\n", "World!");
    printk("1 + 2 = %d, %x\n", 1 + 2, 0x1234abcd);

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

    // Idle loop: "wfi" (Wait For Interrupt) hints the CPU to enter a low-power
    // state until an interrupt occurs. In a minimal kernel with no scheduler,
    // this prevents busy-spinning and reduces power consumption.
    for (;;) {
        __asm__ __volatile__("wfi");
    }
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
