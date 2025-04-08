#include <stdint.h>

#define VGA_ADDRESS 0xB8000
#define CMOS_ADDRESS 0x70
#define CMOS_DATA 0x71
#define VGA_WIDTH 80

volatile uint16_t* vga_buffer = (uint16_t*)VGA_ADDRESS;

void print_at(int x, int y, const char* message) {
    int index = y * VGA_WIDTH + x;
    for (int i = 0; message[i] != '\0'; i++) {
        vga_buffer[index + i] = (0x0F << 8) | message[i];
    }
}

uint8_t read_rtc_register(uint8_t reg) {
    asm volatile ("outb %0, %1" : : "a"(reg), "Nd"(CMOS_ADDRESS));
    uint8_t value;
    asm volatile ("inb %1, %0" : "=a"(value) : "Nd"(CMOS_DATA));
    return value;
}

void get_time(int &hours, int &minutes, int &seconds) {
    hours = read_rtc_register(0x04);
    minutes = read_rtc_register(0x02);
    seconds = read_rtc_register(0x00);
}

extern "C" void kernel_main() {
    while (1) {
        int hours, minutes, seconds;
        get_time(hours, minutes, seconds);

        char time_str[10];
        time_str[0] = '0' + (hours / 10);
        time_str[1] = '0' + (hours % 10);
        time_str[2] = ':';
        time_str[3] = '0' + (minutes / 10);
        time_str[4] = '0' + (minutes % 10);
        time_str[5] = ':';
        time_str[6] = '0' + (seconds / 10);
        time_str[7] = '0' + (seconds % 10);
        time_str[8] = '\0';

        print_at(0, 0, "Time: ");
        print_at(6, 0, time_str);
    }
}
