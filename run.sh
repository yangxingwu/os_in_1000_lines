#!/bin/bash

set -o errexit
set -o nounset
set -o pipefail

QEMU=qemu-system-riscv32

BREW_PREFIX=$(brew --prefix)
LLVM_BIN_PATH="${BREW_PREFIX}"/opt/llvm/bin
CC="${LLVM_BIN_PATH}"/clang

CFLAGS=(
    "-std=c11"
    "-O2"
    "-g3"
    "-Wall"
    "-Wextra"
    "--target=riscv32-unknown-elf" # compile for 32-bit RISC-V
    "-fno-stack-protector" # Disable unnecessary stack protection to avoid unexpected behavior in stack manipulation
    "-ffreestanding" # Do not use the standard library
)
LDFLAGS=(
    "--target=riscv32-unknown-elf"
    "-nostdlib" # do not link the standard library
    "-Wl,-T,kernel.ld" # specify the linker script
    "-Wl,-Map,kernel.map" # output a map file (linker allocation result)
)

"${CC}" "${CFLAGS[@]}" -c -o kernel.o kernel.c
"${CC}" "${LDFLAGS[@]}" kernel.o -o kernel.elf

PROJECT_DIR=$(pwd)
cat > compile_commands.json << EOF
[
    {
        "directory": "${PROJECT_DIR}",
        "command": "${CC} ${CFLAGS[@]} ${LDFLAGS[@]} -o kernel.elf kernel.c",
        "file": "kernel.c"
    }
]
EOF

${QEMU} -machine virt -bios default -nographic -serial mon:stdio --no-reboot \
    -kernel kernel.elf
