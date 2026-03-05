/* =============================================================================
 * MyOS - Kernel Main
 * Entry point after bootloader sets up Protected Mode
 * Initializes all subsystems and hands off to the shell
 * ============================================================================= */

#include "../include/kernel.h"
#include "../include/vga.h"
#include "../include/gdt.h"
#include "../include/idt.h"
#include "../include/memory.h"
#include "../include/scheduler.h"
#include "../include/fs.h"

/* Forward declarations */
void keyboard_init(void);
void timer_init(void);
void task_shell(void);

/* ---------------------------------------------------------------------------
 * kernel_panic - Unrecoverable error
 * --------------------------------------------------------------------------- */
void kernel_panic(const char* msg, const char* file, int line) {
    __asm__ volatile("cli");
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_RED);
    vga_clear();
    kprintf("\n\n");
    kprintf("  !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
    kprintf("  !!         KERNEL PANIC                 !!\n");
    kprintf("  !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
    kprintf("\n");
    kprintf("  Message : %s\n", msg);
    kprintf("  File    : %s\n", file);
    kprintf("  Line    : %d\n", line);
    kprintf("\n  System halted. Please reboot.\n");
    while (1) __asm__ volatile("hlt");
}

/* ---------------------------------------------------------------------------
 * kernel_main - Called from kernel_entry.asm
 * This is where the OS comes alive!
 * --------------------------------------------------------------------------- */
void kernel_main(void) {
    /* -----------------------------------------------------------------------
     * 1. Initialize VGA (must be first - we need output!)
     * ----------------------------------------------------------------------- */
    vga_init();
    vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    kprintf("MyOS Kernel v1.0 - Protected Mode x86\n");
    kprintf("======================================\n");
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);

    /* -----------------------------------------------------------------------
     * 2. Initialize GDT (reload segment descriptors properly)
     * ----------------------------------------------------------------------- */
    kprintf("[BOOT] Initializing GDT...\n");
    gdt_init();
    kprintf("[BOOT] GDT OK (6 entries: null, kcode, kdata, ucode, udata, tss)\n");

    /* -----------------------------------------------------------------------
     * 3. Initialize IDT + PIC (enable interrupt handling)
     * ----------------------------------------------------------------------- */
    kprintf("[BOOT] Initializing IDT + PIC...\n");
    pic_init();
    idt_init();
    kprintf("[BOOT] IDT OK (256 gates, PIC remapped to 0x20-0x2F)\n");

    /* -----------------------------------------------------------------------
     * 4. Initialize Physical Memory Manager
     * ----------------------------------------------------------------------- */
    kprintf("[BOOT] Initializing Physical Memory Manager...\n");
    pmm_init(PHYS_MEM_MAX);
    /* Mark 2MB-16MB as free (skip first 2MB: BIOS, VGA, kernel) */
    pmm_mark_free(0x200000, PHYS_MEM_MAX - 0x200000);
    kprintf("[BOOT] PMM OK: %u KB free (%u pages)\n",
            pmm_free_blocks_count() * 4, pmm_free_blocks_count());

    /* -----------------------------------------------------------------------
     * 5. Initialize Heap
     * ----------------------------------------------------------------------- */
    kprintf("[BOOT] Initializing Kernel Heap...\n");
    heap_init(HEAP_START, HEAP_SIZE);

    /* -----------------------------------------------------------------------
     * 6. Initialize Filesystem
     * ----------------------------------------------------------------------- */
    kprintf("[BOOT] Initializing MYFS Ramdisk...\n");
    fs_init();

    /* -----------------------------------------------------------------------
     * 7. Initialize Scheduler
     * ----------------------------------------------------------------------- */
    kprintf("[BOOT] Initializing Task Scheduler...\n");
    sched_init();

    /* -----------------------------------------------------------------------
     * 8. Initialize Timer (PIT - drives scheduler)
     * ----------------------------------------------------------------------- */
    kprintf("[BOOT] Starting PIT timer...\n");
    timer_init();

    /* -----------------------------------------------------------------------
     * 9. Initialize Keyboard driver
     * ----------------------------------------------------------------------- */
    kprintf("[BOOT] Initializing PS/2 Keyboard...\n");
    keyboard_init();

    /* -----------------------------------------------------------------------
     * 10. Create Shell task
     * ----------------------------------------------------------------------- */
    kprintf("[BOOT] Creating shell task...\n");
    task_t* shell = task_create("shell", task_shell, PRIORITY_NORMAL);
    if (!shell) PANIC("Failed to create shell task");

    /* -----------------------------------------------------------------------
     * 11. Enable interrupts and hand off to shell
     * ----------------------------------------------------------------------- */
    vga_set_color(VGA_COLOR_GREEN, VGA_COLOR_BLACK);
    kprintf("\n[BOOT] All systems GO. Enabling interrupts...\n\n");
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);

    __asm__ volatile("sti");

    /* -----------------------------------------------------------------------
     * 12. Run shell directly (in our simplified single-threaded model)
     *     In a full OS this would be the idle loop with preemptive scheduling
     * ----------------------------------------------------------------------- */
    task_shell();

    /* Should never reach here */
    PANIC("kernel_main returned!");
}
