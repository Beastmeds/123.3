/* =============================================================================
 * MyOS - Interactive Shell
 * A mini bash-like shell running as a kernel task
 * ============================================================================= */

#include "../include/kernel.h"
#include "../include/vga.h"
#include "../include/memory.h"
#include "../include/scheduler.h"
#include "../include/fs.h"
#include "../include/idt.h"

/* Forward declarations */
extern void keyboard_init(void);
extern int  kb_readline(char* buf, int max);
extern int  kb_has_input(void);
extern uint32_t timer_get_ticks(void);
extern void heap_dump(void);
extern void task_ps(void);
extern uint32_t pmm_free_blocks_count(void);
extern uint32_t pmm_total_blocks(void);

/* ---------------------------------------------------------------------------
 * String utilities
 * --------------------------------------------------------------------------- */
static int kstrcmp(const char* a, const char* b) {
    while (*a && *a == *b) { a++; b++; }
    return *(unsigned char*)a - *(unsigned char*)b;
}

static int kstrncmp(const char* a, const char* b, int n) {
    while (n-- && *a && *a == *b) { a++; b++; }
    return n < 0 ? 0 : *(unsigned char*)a - *(unsigned char*)b;
}

static int kstrlen(const char* s) {
    int n = 0; while (*s++) n++; return n;
}

/* ---------------------------------------------------------------------------
 * Command argument parsing
 * --------------------------------------------------------------------------- */
#define MAX_ARGS    8
#define MAX_CMDLEN  256

static int parse_args(char* cmd, char* argv[]) {
    int argc = 0;
    char* p = cmd;
    while (*p && argc < MAX_ARGS) {
        while (*p == ' ') p++;
        if (!*p) break;
        argv[argc++] = p;
        while (*p && *p != ' ') p++;
        if (*p) *p++ = '\0';
    }
    return argc;
}

/* ---------------------------------------------------------------------------
 * Built-in commands
 * --------------------------------------------------------------------------- */

static void cmd_help(int argc, char* argv[]) {
    vga_set_color(VGA_COLOR_YELLOW, VGA_COLOR_BLACK);
    kprintf("\nMyOS Shell Commands:\n");
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    kprintf("  help          Show this help\n");
    kprintf("  clear         Clear screen\n");
    kprintf("  echo [text]   Print text\n");
    kprintf("  ls            List files\n");
    kprintf("  cat [file]    Print file contents\n");
    kprintf("  write [f] [t] Write text to file\n");
    kprintf("  rm [file]     Delete file\n");
    kprintf("  ps            List processes\n");
    kprintf("  meminfo       Memory statistics\n");
    kprintf("  heapdump      Show heap layout\n");
    kprintf("  uptime        System uptime\n");
    kprintf("  colors        Color test\n");
    kprintf("  reboot        Reboot system\n");
    kprintf("  cpuid         CPU information\n");
    kprintf("\n");
}

static void cmd_echo(int argc, char* argv[]) {
    for (int i = 1; i < argc; i++) {
        vga_puts(argv[i]);
        if (i < argc - 1) vga_putchar(' ');
    }
    vga_putchar('\n');
}

static void cmd_ls(int argc, char* argv[]) {
    fs_list();
}

static void cmd_cat(int argc, char* argv[]) {
    if (argc < 2) {
        kprintf("Usage: cat <filename>\n");
        return;
    }
    if (!fs_exists(argv[1])) {
        kprintf("cat: %s: No such file\n", argv[1]);
        return;
    }
    static char buf[FS_MAX_FILESIZE];
    int n = fs_read_file(argv[1], buf, sizeof(buf) - 1);
    if (n < 0) {
        kprintf("cat: read error\n");
        return;
    }
    buf[n] = '\0';
    vga_puts(buf);
    if (n > 0 && buf[n-1] != '\n') vga_putchar('\n');
}

static void cmd_write(int argc, char* argv[]) {
    if (argc < 3) {
        kprintf("Usage: write <filename> <text>\n");
        return;
    }
    /* Reconstruct text from remaining args */
    static char text[256];
    int pos = 0;
    for (int i = 2; i < argc && pos < 254; i++) {
        int len = kstrlen(argv[i]);
        for (int j = 0; j < len && pos < 254; j++) text[pos++] = argv[i][j];
        if (i < argc - 1) text[pos++] = ' ';
    }
    text[pos++] = '\n';
    text[pos]   = '\0';

    int fd = fs_open(argv[1], O_WRITE | O_CREATE | O_APPEND);
    if (fd < 0) {
        kprintf("write: cannot open '%s'\n", argv[1]);
        return;
    }
    fs_write(fd, text, pos);
    fs_close(fd);
    kprintf("Written %d bytes to '%s'\n", pos, argv[1]);
}

static void cmd_rm(int argc, char* argv[]) {
    if (argc < 2) { kprintf("Usage: rm <filename>\n"); return; }
    if (fs_delete(argv[1]) < 0) {
        kprintf("rm: %s: No such file\n", argv[1]);
    } else {
        kprintf("Deleted '%s'\n", argv[1]);
    }
}

static void cmd_meminfo(int argc, char* argv[]) {
    uint32_t free_pages  = pmm_free_blocks_count();
    uint32_t total_pages = pmm_total_blocks();
    uint32_t used_pages  = total_pages - free_pages;

    vga_set_color(VGA_COLOR_CYAN, VGA_COLOR_BLACK);
    kprintf("\n=== Memory Information ===\n");
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    kprintf("  Physical Memory:\n");
    kprintf("    Total:  %u KB (%u pages)\n", total_pages * 4, total_pages);
    kprintf("    Used:   %u KB (%u pages)\n", used_pages  * 4, used_pages);
    kprintf("    Free:   %u KB (%u pages)\n", free_pages  * 4, free_pages);
    kprintf("  Heap: 0x%08X - 0x%08X (%u KB)\n",
            HEAP_START, HEAP_START + HEAP_SIZE, HEAP_SIZE / 1024);
    kprintf("  Page size: %u bytes\n", PAGE_SIZE);
    kprintf("==========================\n\n");
}

static void cmd_uptime(int argc, char* argv[]) {
    uint32_t ticks = timer_get_ticks();
    uint32_t secs  = ticks / 100;
    uint32_t mins  = secs / 60;
    uint32_t hours = mins / 60;
    kprintf("Uptime: %u:%02u:%02u (%u ticks)\n",
            hours, mins % 60, secs % 60, ticks);
}

static void cmd_colors(int argc, char* argv[]) {
    kprintf("\nVGA Color Test:\n");
    for (int bg = 0; bg < 8; bg++) {
        for (int fg = 0; fg < 16; fg++) {
            vga_set_color((vga_color_t)fg, (vga_color_t)bg);
            kprintf(" AB ");
        }
        vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
        kprintf("\n");
    }
}

static void cmd_cpuid(int argc, char* argv[]) {
    uint32_t eax, ebx, ecx, edx;

    /* Get vendor string */
    __asm__ volatile("cpuid"
                     : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
                     : "a"(0));

    char vendor[13];
    *(uint32_t*)(vendor + 0) = ebx;
    *(uint32_t*)(vendor + 4) = edx;
    *(uint32_t*)(vendor + 8) = ecx;
    vendor[12] = '\0';

    kprintf("\nCPU Vendor: %s\n", vendor);
    kprintf("Max CPUID leaf: %u\n", eax);

    /* Get features */
    __asm__ volatile("cpuid"
                     : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
                     : "a"(1));
    kprintf("Family: %u  Model: %u  Stepping: %u\n",
            (eax >> 8) & 0xF, (eax >> 4) & 0xF, eax & 0xF);
    kprintf("Features: %s %s %s %s %s\n",
            (edx & (1<<0))  ? "FPU"  : "",
            (edx & (1<<23)) ? "MMX"  : "",
            (edx & (1<<25)) ? "SSE"  : "",
            (edx & (1<<26)) ? "SSE2" : "",
            (ecx & (1<<0))  ? "SSE3" : "");
}

static void cmd_reboot(int argc, char* argv[]) {
    kprintf("Rebooting...\n");
    /* Keyboard controller reset */
    uint8_t good = 0x02;
    while (good & 0x02) good = inb(0x64);
    outb(0x64, 0xFE);
    /* Triple fault fallback */
    __asm__ volatile("lidt %0; int $3" : : "m"(*(uint16_t*)0));
}

/* ---------------------------------------------------------------------------
 * Shell banner
 * --------------------------------------------------------------------------- */
static void print_banner(void) {
    vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    kprintf("\n");
    kprintf("  ########################################\n");
    kprintf("  #                                      #\n");
    kprintf("  #   ");
    vga_set_color(VGA_COLOR_YELLOW, VGA_COLOR_BLACK);
    kprintf("  MyOS v1.0 - x86 Protected Mode  ");
    vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    kprintf("  #\n");
    kprintf("  #   Kernel Shell Ready             #\n");
    kprintf("  #                                      #\n");
    kprintf("  ########################################\n\n");
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);

    /* Show MOTD */
    if (fs_exists("motd.txt")) {
        static char motd[256];
        int n = fs_read_file("motd.txt", motd, sizeof(motd)-1);
        if (n > 0) {
            motd[n] = '\0';
            vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
            vga_puts(motd);
            vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
        }
    }
    kprintf("Type 'help' for available commands.\n\n");
}

/* ---------------------------------------------------------------------------
 * Shell prompt
 * --------------------------------------------------------------------------- */
static void print_prompt(void) {
    vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    kprintf("root@myos");
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    kprintf(":");
    vga_set_color(VGA_COLOR_LIGHT_BLUE, VGA_COLOR_BLACK);
    kprintf("~");
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    kprintf("# ");
}

/* ---------------------------------------------------------------------------
 * task_shell - Main shell loop (runs as a kernel task)
 * --------------------------------------------------------------------------- */
void task_shell(void) {
    static char  cmdline[MAX_CMDLEN];
    static char* argv[MAX_ARGS];

    print_banner();

    while (1) {
        print_prompt();

        int len = kb_readline(cmdline, MAX_CMDLEN);
        if (len == 0) continue;

        int argc = parse_args(cmdline, argv);
        if (argc == 0) continue;

        /* Dispatch command */
        if      (!kstrcmp(argv[0], "help"))     cmd_help(argc, argv);
        else if (!kstrcmp(argv[0], "clear"))    { vga_clear(); }
        else if (!kstrcmp(argv[0], "echo"))     cmd_echo(argc, argv);
        else if (!kstrcmp(argv[0], "ls"))       cmd_ls(argc, argv);
        else if (!kstrcmp(argv[0], "cat"))      cmd_cat(argc, argv);
        else if (!kstrcmp(argv[0], "write"))    cmd_write(argc, argv);
        else if (!kstrcmp(argv[0], "rm"))       cmd_rm(argc, argv);
        else if (!kstrcmp(argv[0], "ps"))       task_ps();
        else if (!kstrcmp(argv[0], "meminfo"))  cmd_meminfo(argc, argv);
        else if (!kstrcmp(argv[0], "heapdump")) heap_dump();
        else if (!kstrcmp(argv[0], "uptime"))   cmd_uptime(argc, argv);
        else if (!kstrcmp(argv[0], "colors"))   cmd_colors(argc, argv);
        else if (!kstrcmp(argv[0], "cpuid"))    cmd_cpuid(argc, argv);
        else if (!kstrcmp(argv[0], "reboot"))   cmd_reboot(argc, argv);
        else {
            vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
            kprintf("Unknown command: %s\n", argv[0]);
            vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
        }
    }
}
