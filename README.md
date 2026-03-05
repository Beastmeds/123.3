# MyOS — x86 Operating System from Scratch

A fully working hobby OS kernel for x86 (32-bit Protected Mode), built from scratch with no external libraries or frameworks. Boots in QEMU with an interactive shell.

```
  __  __       ___  ____
 |  \/  |_   _/ _ \/ ___|
 | |\/| | | | | | |\___ \
 | |  | | |_| | |_| |___) |
 |_|  |_|\__, |\___/|____/
         |___/  v1.0
```

---

## Architecture Overview

```
+-----------------------------------------+
|           QEMU / Real Hardware          |
+-----------------------------------------+
|   MBR (Sector 0)   Stage 1 Bootloader  |
|   Loads Stage 2 via BIOS INT 13h       |
+-----------------------------------------+
|   Stage 2 Bootloader                   |
|   - Enables A20 line                   |
|   - Gets memory map (E820)             |
|   - Sets up GDT                        |
|   - Switches to 32-bit Protected Mode  |
|   - Loads kernel at 0x10000            |
+-----------------------------------------+
|   Kernel Entry (_start)                |
|   - Zeroes BSS                         |
|   - Calls kernel_main()                |
+-----------------------------------------+
|   kernel_main()                        |
|   1. VGA Driver                        |
|   2. GDT (6 descriptors + TSS)        |
|   3. IDT + PIC (256 gates, IRQs)      |
|   4. Physical Memory Manager          |
|   5. Heap Allocator                   |
|   6. MYFS Ramdisk Filesystem          |
|   7. Task Scheduler                   |
|   8. PIT Timer @ 100 Hz               |
|   9. PS/2 Keyboard Driver             |
|  10. Shell Task                       |
+-----------------------------------------+
|   Interactive Shell (root@myos:~#)     |
+-----------------------------------------+
```

---

## File Structure

```
myos/
├── Makefile              # Build system
├── kernel.ld             # Linker script
├── boot/
│   ├── boot.asm          # Stage 1 bootloader (MBR, 512 bytes)
│   ├── stage2.asm        # Stage 2 (A20, GDT, Protected Mode)
│   └── kernel_entry.asm  # Kernel entry point (zeros BSS, calls kernel_main)
├── include/
│   ├── kernel.h          # Core types, port I/O, constants
│   ├── vga.h             # VGA text mode driver
│   ├── gdt.h             # GDT structures
│   ├── idt.h             # IDT structures, interrupt types
│   ├── memory.h          # PMM + heap allocator
│   ├── scheduler.h       # Task scheduler
│   └── fs.h              # Ramdisk filesystem
├── kernel/
│   ├── kernel.c          # kernel_main() — boots all subsystems
│   ├── gdt.c             # GDT initialization (6 segments)
│   ├── gdt_flush.asm     # lgdt + segment register reload
│   ├── idt.c             # IDT + PIC + exception/IRQ/syscall handlers
│   ├── idt_stubs.asm     # 48 ISR/IRQ assembly stubs
│   ├── memory.c          # PMM bitmap + heap block allocator
│   ├── scheduler.c       # Round-robin preemptive scheduler
│   └── shell.c           # Interactive shell (10+ commands)
├── drivers/
│   ├── vga.c             # VGA 80x25 text mode + kprintf()
│   ├── keyboard.c        # PS/2 keyboard driver + ring buffer
│   └── timer.c           # PIT @ 100Hz, drives scheduler
└── fs/
    └── fs.c              # MYFS ramdisk filesystem
```

---

## Build & Run

### Prerequisites

```bash
# Ubuntu/Debian
sudo apt-get install nasm gcc qemu-system-x86 make

# Arch Linux
sudo pacman -S nasm gcc qemu make

# macOS (Homebrew)
brew install nasm qemu
brew install x86_64-elf-gcc  # Cross compiler
```

### Build

```bash
make
```

This creates `myos.img` — a 1.44MB floppy disk image.

### Run

```bash
make run       # QEMU with curses display (terminal)
make run-sdl   # QEMU with SDL window
make run-debug # QEMU + serial output + interrupt logging
```

---

## Shell Commands

| Command | Description |
|---------|-------------|
| `help` | Show all commands |
| `clear` | Clear screen |
| `echo [text]` | Print text |
| `ls` | List files in MYFS |
| `cat [file]` | Print file contents |
| `write [file] [text]` | Write text to file |
| `rm [file]` | Delete file |
| `ps` | List running processes |
| `meminfo` | Physical memory + heap stats |
| `heapdump` | Show heap block layout |
| `uptime` | System uptime |
| `colors` | VGA color palette test |
| `cpuid` | CPU vendor + feature flags |
| `reboot` | Restart system |

---

## Key Concepts Implemented

### Bootloader (2-stage)
- **Stage 1** (MBR, 512 bytes): BIOS INT 13h disk read, loads Stage 2
- **Stage 2**: A20 line enable, BIOS E820 memory map, GDT setup, switches to 32-bit Protected Mode

### GDT (Global Descriptor Table)
- 6 entries: null, kernel code (Ring 0), kernel data (Ring 0), user code (Ring 3), user data (Ring 3), TSS
- Proper 4KB granularity, 32-bit protected mode descriptors

### IDT (Interrupt Descriptor Table)
- 256 gates: 32 CPU exceptions, 16 hardware IRQs (0x20-0x2F), syscall (INT 0x80)
- PIC remapping from legacy 0x08-0x0F/0x70-0x77 to 0x20-0x2F
- Assembly stubs save full register state (registers_t), call C handlers

### Physical Memory Manager
- Bitmap allocator: 1 bit per 4KB page
- O(n) first-free search, O(1) free
- Supports contiguous multi-page allocations

### Heap Allocator
- Linked-list block allocator with header magic (0xDEADBEEF)
- First-fit allocation, bi-directional coalescing
- `kmalloc`, `kcalloc`, `krealloc`, `kfree`

### Task Scheduler
- Round-robin with priority levels (LOW=1, NORMAL=2, HIGH=4, KERNEL=8)
- `task_create`, `task_destroy`, `task_sleep`, `task_yield`, `task_exit`
- Timer-driven preemption at 100Hz
- Task Control Block with saved register state

### MYFS Ramdisk Filesystem
- Flat filesystem in 1MB ramdisk
- Up to 64 files, 64KB per file
- `open`, `close`, `read`, `write`, `seek`, `create`, `delete`
- Pre-populated with `readme.txt` and `motd.txt`

### Syscall Interface
- INT 0x80 gateway (like Linux)
- `sys_exit` (0), `sys_write` (1)
- Ring 3 callable (DPL=3 IDT gate)

### VGA Driver
- 80x25 color text mode
- Hardware cursor control (I/O port 0x3D4/0x3D5)
- Scrolling, tab, backspace
- Full `kprintf` with `%d`, `%u`, `%x`, `%X`, `%s`, `%c`, `%p`

### PS/2 Keyboard
- Scancode Set 1 to ASCII mapping
- Shift, Caps Lock, Ctrl modifier tracking
- Ring buffer (256 bytes) with IRQ1 handler

---

## Extending MyOS

### Add a new syscall
```c
// In kernel/idt.c, syscall_handler():
case 2: /* sys_read */
    // EBX = fd, ECX = buf, EDX = len
    regs->eax = fs_read(regs->ebx, (void*)regs->ecx, regs->edx);
    break;
```

### Add paging (next big step!)
```c
// Create page directory + page tables
// Load CR3 with page directory physical address
// Set bit 31 of CR0 to enable paging
uint32_t cr0;
__asm__("mov %%cr0, %0" : "=r"(cr0));
cr0 |= 0x80000000;
__asm__("mov %0, %%cr0" :: "r"(cr0));
```

### Load user programs (ELF loader)
```c
// Parse ELF header, load segments into memory
// Switch to user mode with IRET trick:
// Push SS, ESP, EFLAGS, CS (user), EIP onto stack
// iret jumps to user code at Ring 3
```

---

## Disk Image Layout

```
Offset      Size    Content
0x000000    512B    Stage 1 Bootloader (MBR)
0x000200    2KB     Stage 2 Bootloader
0x000A00    32KB    Kernel Binary
...         ...     (rest is zeros / filesystem data in RAM)
```

---

## Debugging Tips

```bash
# View QEMU interrupt log
make run-debug

# Disassemble kernel
make disasm

# GDB remote debugging
make run-gdb
# In GDB: break kernel_main, continue
```

---

## References / Further Reading

- **OSDev Wiki**: https://wiki.osdev.org — the bible of hobby OS development
- **Intel IA-32 Manuals**: The authoritative reference for x86 instructions and hardware
- **"Writing a Simple OS from Scratch"** by Nick Blundell (free PDF)
- **xv6**: MIT's educational Unix-like OS (great reference for syscalls + scheduling)
- **Linux 0.01**: Linus Torvalds' original 10,000-line kernel

---

*Built with: NASM + GCC + LD + QEMU*
