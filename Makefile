# =============================================================================
# MyOS - Makefile
# Builds bootloader, kernel, and creates a bootable disk image
# =============================================================================

# --- Tools ---
AS      = nasm
CC      = gcc
LD      = ld
OBJCOPY = objcopy

# --- Flags ---
ASFLAGS  = -f elf32
ASFLAGSBIN = -f bin
CCFLAGS  = -m32 -ffreestanding -fno-stack-protector -fno-pic \
           -nostdlib -nostdinc -fno-builtin \
           -I./include \
           -Wall -Wextra -Wno-unused-parameter \
           -O2
LDFLAGS  = -m elf_i386 -T kernel.ld --oformat binary

# --- Output ---
BUILD   = build
DISK    = myos.img
BOOT    = $(BUILD)/boot.bin
STAGE2  = $(BUILD)/stage2.bin
KERNEL  = $(BUILD)/kernel.bin

# --- Sources ---
BOOT_SRC   = boot/boot.asm
STAGE2_SRC = boot/stage2.asm

KERNEL_ASM_SRCS = \
	boot/kernel_entry.asm \
	kernel/gdt_flush.asm \
	kernel/idt_stubs.asm

KERNEL_C_SRCS = \
	kernel/kernel.c \
	kernel/gdt.c \
	kernel/idt.c \
	kernel/memory.c \
	kernel/scheduler.c \
	kernel/shell.c \
	drivers/vga.c \
	drivers/keyboard.c \
	drivers/timer.c \
	fs/fs.c

KERNEL_ASM_OBJS = $(patsubst %.asm, $(BUILD)/%.o, $(KERNEL_ASM_SRCS))
KERNEL_C_OBJS   = $(patsubst %.c,   $(BUILD)/%.o, $(KERNEL_C_SRCS))
KERNEL_OBJS     = $(KERNEL_ASM_OBJS) $(KERNEL_C_OBJS)

# =============================================================================
# Default target
# =============================================================================
all: $(DISK)
	@echo ""
	@echo "  ============================================"
	@echo "  Build complete! Disk image: $(DISK)"
	@echo "  Run with: make run"
	@echo "  ============================================"

# =============================================================================
# Create bootable disk image
# Layout:
#   Sector 1:     Stage 1 bootloader (MBR, 512 bytes)
#   Sectors 2-5:  Stage 2 bootloader (4 sectors = 2KB)
#   Sectors 6-69: Kernel (64 sectors = 32KB)
# =============================================================================
$(DISK): $(BOOT) $(STAGE2) $(KERNEL)
	@echo "[DISK] Creating disk image $@"
	# Create 1.44MB floppy-style image filled with zeros
	dd if=/dev/zero of=$(DISK) bs=512 count=2880 2>/dev/null
	# Write Stage 1 at sector 0
	dd if=$(BOOT) of=$(DISK) bs=512 count=1 conv=notrunc 2>/dev/null
	# Write Stage 2 at sector 1
	dd if=$(STAGE2) of=$(DISK) bs=512 seek=1 count=4 conv=notrunc 2>/dev/null
	# Write kernel at sector 5
	dd if=$(KERNEL) of=$(DISK) bs=512 seek=5 conv=notrunc 2>/dev/null
	@echo "[DISK] Image size: $$(du -h $(DISK) | cut -f1)"

# =============================================================================
# Bootloader
# =============================================================================
$(BOOT): $(BOOT_SRC) | $(BUILD)/boot
	@echo "[AS]  $<"
	$(AS) $(ASFLAGSBIN) $< -o $@

$(STAGE2): $(STAGE2_SRC) | $(BUILD)/boot
	@echo "[AS]  $<"
	$(AS) $(ASFLAGSBIN) $< -o $@

# =============================================================================
# Kernel - Assembly objects
# =============================================================================
$(BUILD)/boot/kernel_entry.o: boot/kernel_entry.asm | $(BUILD)/boot
	@echo "[AS]  $<"
	$(AS) $(ASFLAGS) $< -o $@

$(BUILD)/kernel/gdt_flush.o: kernel/gdt_flush.asm | $(BUILD)/kernel
	@echo "[AS]  $<"
	$(AS) $(ASFLAGS) $< -o $@

$(BUILD)/kernel/idt_stubs.o: kernel/idt_stubs.asm | $(BUILD)/kernel
	@echo "[AS]  $<"
	$(AS) $(ASFLAGS) $< -o $@

# =============================================================================
# Kernel - C objects
# =============================================================================
$(BUILD)/kernel/%.o: kernel/%.c | $(BUILD)/kernel
	@echo "[CC]  $<"
	$(CC) $(CCFLAGS) -c $< -o $@

$(BUILD)/drivers/%.o: drivers/%.c | $(BUILD)/drivers
	@echo "[CC]  $<"
	$(CC) $(CCFLAGS) -c $< -o $@

$(BUILD)/fs/%.o: fs/%.c | $(BUILD)/fs
	@echo "[CC]  $<"
	$(CC) $(CCFLAGS) -c $< -o $@

# =============================================================================
# Link kernel
# =============================================================================
$(KERNEL): $(KERNEL_OBJS) kernel.ld | $(BUILD)
	@echo "[LD]  Linking kernel..."
	$(LD) $(LDFLAGS) -o $@ $(KERNEL_OBJS)
	@echo "[LD]  Kernel size: $$(du -h $@ | cut -f1)"

# =============================================================================
# Create build directories
# =============================================================================
$(BUILD)/boot:    ; @mkdir -p $@
$(BUILD)/kernel:  ; @mkdir -p $@
$(BUILD)/drivers: ; @mkdir -p $@
$(BUILD)/fs:      ; @mkdir -p $@
$(BUILD):         ; @mkdir -p $@

# =============================================================================
# Run in QEMU
# =============================================================================
run: $(DISK)
	@echo "[QEMU] Booting MyOS..."
	qemu-system-i386 \
		-fda $(DISK) \
		-m 32 \
		-display curses \
		-no-reboot \
		-no-shutdown

# Run with QEMU in window (if display available)
run-sdl: $(DISK)
	qemu-system-i386 \
		-fda $(DISK) \
		-m 32 \
		-display sdl \
		-no-reboot

# Run with serial output for debugging
run-debug: $(DISK)
	qemu-system-i386 \
		-fda $(DISK) \
		-m 32 \
		-display curses \
		-serial stdio \
		-d int,cpu_reset \
		-D qemu.log \
		-no-reboot

# GDB debug session
run-gdb: $(DISK)
	qemu-system-i386 \
		-fda $(DISK) \
		-m 32 \
		-display curses \
		-s -S &
	gdb \
		-ex "target remote localhost:1234" \
		-ex "set architecture i386" \
		-ex "symbol-file $(BUILD)/kernel.elf"

# =============================================================================
# Utilities
# =============================================================================
clean:
	@echo "[CLN] Cleaning build artifacts..."
	rm -rf $(BUILD) $(DISK) qemu.log

# Show disk layout
disk-info: $(DISK)
	@echo "Disk layout:"
	@echo "  Sector 0:    MBR / Stage1 ($(shell du -b $(BOOT) | cut -f1) bytes)"
	@echo "  Sectors 1-4: Stage2 ($(shell du -b $(STAGE2) | cut -f1) bytes)"
	@echo "  Sectors 5+:  Kernel ($(shell du -b $(KERNEL) | cut -f1) bytes)"
	@hexdump -C $(DISK) | head -4

# Disassemble kernel (useful for debugging)
disasm: $(KERNEL)
	ndisasm -b 32 -o 0x10000 $(KERNEL) | head -100

.PHONY: all run run-sdl run-debug run-gdb clean disk-info disasm
