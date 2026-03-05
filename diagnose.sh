#!/bin/bash

echo "=== Checking prerequisites ==="
which nasm gcc ld || echo "Missing tools!"

echo -e "\n=== Checking file existence ==="
for file in boot/boot.asm boot/stage2.asm boot/kernel_entry.asm kernel/gdt.c kernel/kernel.c kernel/gdt_flush.asm kernel/idt_stubs.asm kernel.ld; do
    if [ -f "$file" ]; then
        echo "✓ $file"
    else
        echo "✗ MISSING: $file"
    fi
done

echo -e "\n=== Checking build dirs ==="
mkdir -p build/boot build/kernel build/drivers build/fs
echo "✓ Build directories created"

echo -e "\n=== Test: Compile boot.asm ==="
nasm -f bin boot/boot.asm -o build/boot.bin 2>&1 && echo "✓ boot.asm compiled" || echo "✗ boot.asm ERROR"

echo -e "\n=== Test: Compile stage2.asm ==="
nasm -f bin boot/stage2.asm -o build/stage2.bin 2>&1 && echo "✓ stage2.asm compiled" || echo "✗ stage2.asm ERROR"

echo -e "\n=== Test: Compile kernel_entry.asm ==="
nasm -f elf32 boot/kernel_entry.asm -o build/boot/kernel_entry.o 2>&1 && echo "✓ kernel_entry.asm compiled" || echo "✗ kernel_entry.asm ERROR"

echo -e "\n=== Test: Compile gdt.c ==="
gcc -m32 -ffreestanding -fno-stack-protector  -I./include -c kernel/gdt.c -o build/kernel/gdt.o 2>&1 && echo "✓ gdt.c compiled" || echo "✗ gdt.c ERROR"

echo -e "\n=== Test: Compile kernel.c ==="
gcc -m32 -ffreestanding -fno-stack-protector -I./include -c kernel/kernel.c -o build/kernel/kernel.o 2>&1 && echo "✓ kernel.c compiled" || echo "✗ kernel.c ERROR"

echo -e "\n=== Test: Compile vga.c ==="
gcc -m32 -ffreestanding -fno-stack-protector -I./include -c drivers/vga.c -o build/drivers/vga.o 2>&1 && echo "✓ vga.c compiled" || echo "✗ vga.c ERROR"

echo -e "\n=== Testing full make ==="
cd /workspaces/123.3
make clean 2>/dev/null
make 2>&1 | head -100
make_result=$?
echo -e "\nMake exit code: $make_result"
if [ $make_result -eq 0 ]; then
    echo "✓✓✓ BUILD SUCCESSFUL ✓✓✓"
else
    echo "✗✗✗ BUILD FAILED ✗✗✗"
fi
