#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>

/* Protocol Commands - must match firmware */
#define CMD_LED_OFF       0x00
#define CMD_LED_RED       0x01
#define CMD_LED_GREEN     0x02
#define CMD_LED_BLUE      0x03
#define CMD_LED_YELLOW    0x04
#define CMD_LED_CYAN      0x05
#define CMD_LED_MAGENTA   0x06
#define CMD_LED_WHITE     0x07
#define CMD_LED_CUSTOM    0x08

/* Events */
#define EVENT_BTN_LEFT_PRESS    0x10
#define EVENT_BTN_LEFT_RELEASE  0x11
#define EVENT_BTN_RIGHT_PRESS   0x20
#define EVENT_BTN_RIGHT_RELEASE 0x21
#define EVENT_BTN_MID_PRESS     0x30
#define EVENT_BTN_MID_RELEASE   0x31

static volatile int keep_running = 1;

void signal_handler(int sig) {
    keep_running = 0;
}

void print_usage(const char *prog) {
    printf("\n");
    printf("╔═══════════════════════════════════════════════════════╗\n");
    printf("║   🖱️  Pico Mouse Joystick - Control Application      ║\n");
    printf("╚═══════════════════════════════════════════════════════╝\n");
    printf("\n");
    printf("Usage: %s <command> [args]\n\n", prog);
    printf("LED Commands:\n");
    printf("  off              - Turn LED off\n");
    printf("  red              - Set LED to red\n");
    printf("  green            - Set LED to green\n");
    printf("  blue             - Set LED to blue\n");
    printf("  yellow           - Set LED to yellow\n");
    printf("  cyan             - Set LED to cyan\n");
    printf("  magenta          - Set LED to magenta\n");
    printf("  white            - Set LED to white\n");
    printf("  custom R G B     - Set custom RGB color (0-255)\n");
    printf("\n");
    printf("Monitoring Commands:\n");
    printf("  monitor          - Monitor button events (Ctrl+C to stop)\n");
    printf("  test             - Run LED color test sequence\n");
    printf("\n");
    printf("Examples:\n");
    printf("  %s red\n", prog);
    printf("  %s custom 128 0 255\n", prog);
    printf("  %s monitor\n", prog);
    printf("\n");
}

const char* event_to_string(unsigned char event) {
    switch (event) {
        case EVENT_BTN_LEFT_PRESS:    return "LEFT BUTTON PRESSED";
        case EVENT_BTN_LEFT_RELEASE:  return "LEFT BUTTON RELEASED";
        case EVENT_BTN_RIGHT_PRESS:   return "RIGHT BUTTON PRESSED";
        case EVENT_BTN_RIGHT_RELEASE: return "RIGHT BUTTON RELEASED";
        case EVENT_BTN_MID_PRESS:     return "MIDDLE BUTTON PRESSED";
        case EVENT_BTN_MID_RELEASE:   return "MIDDLE BUTTON RELEASED";
        default:                      return "UNKNOWN EVENT";
    }
}

int send_led_command(int fd, unsigned char cmd, unsigned char r, unsigned char g, unsigned char b) {
    unsigned char buf[4];
    int ret;
    
    buf[0] = cmd;
    
    if (cmd == CMD_LED_CUSTOM) {
        buf[1] = r;
        buf[2] = g;
        buf[3] = b;
        ret = write(fd, buf, 4);
    } else {
        ret = write(fd, buf, 1);
    }
    
    if (ret < 0) {
        perror("write");
        return -1;
    }
    
    return 0;
}

int monitor_events(int fd) {
    unsigned char buf[64];
    int ret;
    
    signal(SIGINT, signal_handler);
    
    printf("\n");
    printf("╔═══════════════════════════════════════════════════════╗\n");
    printf("║   📡 Monitoring Button Events (Press Ctrl+C to stop)  ║\n");
    printf("╚═══════════════════════════════════════════════════════╝\n");
    printf("\n");
    printf("Try clicking the buttons on your Pico Mouse...\n\n");
    
    while (keep_running) {
        ret = read(fd, buf, sizeof(buf));
        
        if (ret < 0) {
            if (errno == EAGAIN || errno == ETIMEDOUT) {
                usleep(10000);
                continue;
            }
            perror("read");
            return -1;
        }
        
        if (ret > 0) {
            printf("[EVENT] 0x%02X - %s\n", buf[0], event_to_string(buf[0]));
            fflush(stdout);
        }
    }
    
    printf("\nMonitoring stopped.\n");
    return 0;
}

int run_test_sequence(int fd) {
    printf("\n");
    printf("╔═══════════════════════════════════════════════════════╗\n");
    printf("║   🎨 Running LED Color Test Sequence                  ║\n");
    printf("╚═══════════════════════════════════════════════════════╝\n");
    printf("\n");
    
    struct {
        const char *name;
        unsigned char cmd;
        unsigned char r, g, b;
    } tests[] = {
        {"Red",     CMD_LED_RED,     255, 0,   0  },
        {"Green",   CMD_LED_GREEN,   0,   255, 0  },
        {"Blue",    CMD_LED_BLUE,    0,   0,   255},
        {"Yellow",  CMD_LED_YELLOW,  255, 255, 0  },
        {"Cyan",    CMD_LED_CYAN,    0,   255, 255},
        {"Magenta", CMD_LED_MAGENTA, 255, 0,   255},
        {"White",   CMD_LED_WHITE,   255, 255, 255},
        {"Off",     CMD_LED_OFF,     0,   0,   0  },
    };
    
    for (int i = 0; i < sizeof(tests) / sizeof(tests[0]); i++) {
        printf("  [%d/%d] Setting LED to %s...\n", 
               i + 1, (int)(sizeof(tests) / sizeof(tests[0])), tests[i].name);
        
        if (send_led_command(fd, tests[i].cmd, tests[i].r, tests[i].g, tests[i].b) < 0) {
            fprintf(stderr, "Error: Failed to set LED to %s\n", tests[i].name);
            return -1;
        }
        
        sleep(1);
    }
    
    printf("\n✅ Test sequence completed!\n\n");
    return 0;
}

int main(int argc, char *argv[]) {
    int fd;
    int ret = 0;
    
    /* Try to open device */
    fd = open("/dev/pico_mouse0", O_RDWR);
    if (fd < 0) {
        fd = open("/dev/pico_mouse", O_RDWR);
        if (fd < 0) {
            perror("open");
            fprintf(stderr, "\n");
            fprintf(stderr, "Error: Could not open device.\n");
            fprintf(stderr, "Please check:\n");
            fprintf(stderr, "  1. Pico is connected\n");
            fprintf(stderr, "  2. Driver is loaded: lsmod | grep pico_mouse\n");
            fprintf(stderr, "  3. Device exists: ls -l /dev/pico_mouse*\n");
            fprintf(stderr, "  4. You have permissions (run as root or configure udev)\n");
            fprintf(stderr, "\n");
            return 1;
        }
    }
    
    printf("✅ Device opened successfully (fd=%d)\n", fd);
    
    /* Parse command */
    if (argc < 2) {
        print_usage(argv[0]);
        close(fd);
        return 0;
    }
    
    /* Execute command */
    if (strcmp(argv[1], "off") == 0) {
        printf("💡 Turning LED off...\n");
        ret = send_led_command(fd, CMD_LED_OFF, 0, 0, 0);
    }
    else if (strcmp(argv[1], "red") == 0) {
        printf("💡 Setting LED to RED...\n");
        ret = send_led_command(fd, CMD_LED_RED, 0, 0, 0);
    }
    else if (strcmp(argv[1], "green") == 0) {
        printf("💡 Setting LED to GREEN...\n");
        ret = send_led_command(fd, CMD_LED_GREEN, 0, 0, 0);
    }
    else if (strcmp(argv[1], "blue") == 0) {
        printf("💡 Setting LED to BLUE...\n");
        ret = send_led_command(fd, CMD_LED_BLUE, 0, 0, 0);
    }
    else if (strcmp(argv[1], "yellow") == 0) {
        printf("💡 Setting LED to YELLOW...\n");
        ret = send_led_command(fd, CMD_LED_YELLOW, 0, 0, 0);
    }
    else if (strcmp(argv[1], "cyan") == 0) {
        printf("💡 Setting LED to CYAN...\n");
        ret = send_led_command(fd, CMD_LED_CYAN, 0, 0, 0);
    }
    else if (strcmp(argv[1], "magenta") == 0) {
        printf("💡 Setting LED to MAGENTA...\n");
        ret = send_led_command(fd, CMD_LED_MAGENTA, 0, 0, 0);
    }
    else if (strcmp(argv[1], "white") == 0) {
        printf("💡 Setting LED to WHITE...\n");
        ret = send_led_command(fd, CMD_LED_WHITE, 0, 0, 0);
    }
    else if (strcmp(argv[1], "custom") == 0) {
        if (argc < 5) {
            fprintf(stderr, "Error: custom command requires R G B values (0-255)\n");
            fprintf(stderr, "Example: %s custom 128 0 255\n", argv[0]);
            close(fd);
            return 1;
        }
        
        int r = atoi(argv[2]);
        int g = atoi(argv[3]);
        int b = atoi(argv[4]);
        
        if (r < 0 || r > 255 || g < 0 || g > 255 || b < 0 || b > 255) {
            fprintf(stderr, "Error: RGB values must be between 0 and 255\n");
            close(fd);
            return 1;
        }
        
        printf("💡 Setting LED to custom color RGB(%d, %d, %d)...\n", r, g, b);
        ret = send_led_command(fd, CMD_LED_CUSTOM, r, g, b);
    }
    else if (strcmp(argv[1], "monitor") == 0) {
        ret = monitor_events(fd);
    }
    else if (strcmp(argv[1], "test") == 0) {
        ret = run_test_sequence(fd);
    }
    else {
        fprintf(stderr, "Error: Unknown command '%s'\n", argv[1]);
        print_usage(argv[0]);
        close(fd);
        return 1;
    }
    
    if (ret == 0 && strcmp(argv[1], "monitor") != 0 && strcmp(argv[1], "test") != 0) {
        printf("✅ Command executed successfully!\n");
    }
    
    close(fd);
    return ret;
}