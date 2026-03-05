/* =============================================================================
 * MyOS - VGA Driver
 * Text mode video output
 * ============================================================================= */

#include "../include/vga.h"
#include <stdarg.h>

#define VGA_MEMORY   0xB8000
#define VGA_WIDTH    80
#define VGA_HEIGHT   25
#define VGA_ATTR     0x0F

static uint16_t* vga_buffer = (uint16_t*)VGA_MEMORY;
static uint8_t cursor_x = 0;
static uint8_t cursor_y = 0;
static uint8_t current_color = 0x0F;

void vga_init(void) {
    vga_clear();
}

void vga_clear(void) {
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        vga_buffer[i] = (current_color << 8) | ' ';
    }
    cursor_x = 0;
    cursor_y = 0;
}

void vga_set_color(uint8_t fg, uint8_t bg) {
    current_color = (bg << 4) | (fg & 0x0F);
}

static void vga_scroll(void) {
    for (int i = 0; i < VGA_WIDTH * (VGA_HEIGHT - 1); i++) {
        vga_buffer[i] = vga_buffer[i + VGA_WIDTH];
    }
    for (int i = VGA_WIDTH * (VGA_HEIGHT - 1); i < VGA_WIDTH * VGA_HEIGHT; i++) {
        vga_buffer[i] = (current_color << 8) | ' ';
    }
    cursor_y = VGA_HEIGHT - 1;
}

void vga_putc(char c) {
    if (c == '\n') {
        cursor_x = 0;
        cursor_y++;
    } else if (c == '\t') {
        cursor_x += 4;
    } else {
        int pos = cursor_y * VGA_WIDTH + cursor_x;
        vga_buffer[pos] = (current_color << 8) | c;
        cursor_x++;
    }
    
    if (cursor_x >= VGA_WIDTH) {
        cursor_x = 0;
        cursor_y++;
    }
    
    if (cursor_y >= VGA_HEIGHT) {
        vga_scroll();
    }
}

int kprintf(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    
    int count = 0;
    for (const char* p = fmt; *p; p++) {
        if (*p == '%') {
            p++;
            if (*p == 'd') {
                int val = va_arg(args, int);
                char buf[16];
                int len = 0, temp = val;
                if (val == 0) buf[len++] = '0';
                else {
                    if (val < 0) { vga_putc('-'); val = -val; count++; }
                    int divisor = 1;
                    while (divisor <= val) divisor *= 10;
                    divisor /= 10;
                    while (divisor > 0) {
                        buf[len++] = '0' + (val / divisor);
                        val %= divisor;
                        divisor /= 10;
                    }
                }
                for (int i = 0; i < len; i++) {
                    vga_putc(buf[i]);
                    count++;
                }
            } else if (*p == 'x') {
                uint32_t val = va_arg(args, uint32_t);
                const char* hex = "0123456789abcdef";
                char buf[16];
                int len = 0;
                if (val == 0) buf[len++] = '0';
                else {
                    uint32_t temp = val;
                    while (temp) { buf[len++] = hex[temp & 0xF]; temp >>= 4; }
                }
                for (int i = len - 1; i >= 0; i--) {
                    vga_putc(buf[i]);
                    count++;
                }
            } else if (*p == 's') {
                const char* str = va_arg(args, const char*);
                while (*str) {
                    vga_putc(*str++);
                    count++;
                }
            } else if (*p == '%') {
                vga_putc('%');
                count++;
            }
        } else {
            vga_putc(*p);
            count++;
        }
    }
    
    va_end(args);
    return count;
}
