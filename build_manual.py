#!/usr/bin/env python3

import subprocess
import os
import sys

os.chdir('/workspaces/123.3')

# Create build dirs
os.makedirs('build/boot', exist_ok=True)
os.makedirs('build/kernel', exist_ok=True)
os.makedirs('build/drivers', exist_ok=True)
os.makedirs('build/fs', exist_ok=True)

def run_cmd(cmd, desc):
    print(f"\n[*] {desc}")
    print(f"    {' '.join(cmd)}")
    result = subprocess.run(cmd, capture_output=True, text=True)
    if result.returncode != 0:
        print(f"    [✗] FAILED")
        if result.stderr:
            print(f"    ERROR: {result.stderr[:500]}")
        if result.stdout:
            print(f"    OUTPUT: {result.stdout[:500]}")
        return False
    else:
        print(f"    [✓] OK")
        return True

all_ok = True

# Assemble bootloader
all_ok &= run_cmd(['nasm', '-f', 'bin', 'boot/boot.asm', '-o', 'build/boot.bin'], 'Assemble boot.asm')
all_ok &= run_cmd(['nasm', '-f', 'bin', 'boot/stage2.asm', '-o', 'build/stage2.bin'], 'Assemble stage2.asm')

# Assemble kernel ASM
all_ok &= run_cmd(['nasm', '-f', 'elf32', 'boot/kernel_entry.asm', '-o', 'build/boot/kernel_entry.o'], 'Assemble kernel_entry.asm')
all_ok &= run_cmd(['nasm', '-f', 'elf32', 'kernel/gdt_flush.asm', '-o', 'build/kernel/gdt_flush.o'], 'Assemble gdt_flush.asm')
all_ok &= run_cmd(['nasm', '-f', 'elf32', 'kernel/idt_stubs.asm', '-o', 'build/kernel/idt_stubs.o'], 'Assemble idt_stubs.asm')

# Compile C files
cc_files = [
    ('kernel/gdt.c', 'build/kernel/gdt.o'),
    ('kernel/idt.c', 'build/kernel/idt.o'),
    ('kernel/memory.c', 'build/kernel/memory.o'),
    ('kernel/scheduler.c', 'build/kernel/scheduler.o'),
    ('kernel/kernel.c', 'build/kernel/kernel.o'),
    ('kernel/shell.c', 'build/kernel/shell.o'),
    ('drivers/vga.c', 'build/drivers/vga.o'),
    ('drivers/keyboard.c', 'build/drivers/keyboard.o'),
    ('drivers/timer.c', 'build/drivers/timer.o'),
    ('fs/fs.c', 'build/fs/fs.o'),
]

for src, obj in cc_files:
    all_ok &= run_cmd(['gcc', '-m32', '-ffreestanding', '-fno-stack-protector', '-I./include', '-O0', '-c', src, '-o', obj], f'Compile {src}')

if not all_ok:
    print("\n[✗✗✗] Build failed during compilation!")
    sys.exit(1)

# Link kernel
obj_files = [
    'build/boot/kernel_entry.o',
    'build/kernel/gdt_flush.o',
    'build/kernel/idt_stubs.o',
    'build/kernel/kernel.o',
    'build/kernel/gdt.o',
    'build/kernel/idt.o',
    'build/kernel/memory.o',
    'build/kernel/scheduler.o',
    'build/kernel/shell.o',
    'build/drivers/vga.o',
    'build/drivers/keyboard.o',
    'build/drivers/timer.o',
    'build/fs/fs.o',
]

all_ok &= run_cmd(
    ['ld', '-m', 'elf_i386', '-e', '_start', '-T', 'kernel.ld', '--oformat', 'binary', '-n', '-o', 'build/kernel.bin'] + obj_files,
    'Link kernel'
)

if not all_ok:
    print("\n[✗✗✗] Build failed during linking!")
    sys.exit(1)

# Create disk image
print("\n[*] Creating disk image")
os.system('dd if=/dev/zero of=myos.img bs=512 count=2880 2>/dev/null')
os.system('dd if=build/boot.bin of=myos.img bs=512 count=1 conv=notrunc 2>/dev/null')
os.system('dd if=build/stage2.bin of=myos.img bs=512 seek=1 count=4 conv=notrunc 2>/dev/null')
os.system('dd if=build/kernel.bin of=myos.img bs=512 seek=5 conv=notrunc 2>/dev/null')

size = os.path.getsize('myos.img')
print(f"[✓] myos.img created ({size} bytes)")

print("\n[✓✓✓] BUILD SUCCESSFUL! [✓✓✓]")
print("     Run: qemu-system-i386 -fda myos.img -m 32 -display curses")
