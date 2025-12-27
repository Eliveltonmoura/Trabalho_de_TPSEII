#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "tusb.h"
#include "hardware/adc.h"
#include "hardware/pwm.h"

// ================= CONFIGURAÇÃO =================
#define BUTTON_LEFT_PIN    10  // Botão A = Clique Esquerdo
#define BUTTON_RIGHT_PIN    5  // Botão B = Clique Direito
#define BUTTON_MIDDLE_PIN   6  // Botão Joystick = Clique Meio
#define LED_RED_PIN        13
#define LED_GREEN_PIN      11
#define LED_BLUE_PIN       12
#define JOYSTICK_X_PIN     26
#define JOYSTICK_Y_PIN     27
#define DEADZONE          150
#define SENSITIVITY        20
#define MAX_SPEED         127
#define POLLING_RATE       30

// ================= PROTOCOLO VENDOR =================
#define CMD_LED_OFF       0x00
#define CMD_LED_RED       0x01
#define CMD_LED_GREEN     0x02
#define CMD_LED_BLUE      0x03
#define CMD_LED_YELLOW    0x04
#define CMD_LED_CYAN      0x05
#define CMD_LED_MAGENTA   0x06
#define CMD_LED_WHITE     0x07
#define CMD_LED_CUSTOM    0x08

#define EVENT_BTN_LEFT_PRESS    0x10
#define EVENT_BTN_LEFT_RELEASE  0x11
#define EVENT_BTN_RIGHT_PRESS   0x20
#define EVENT_BTN_RIGHT_RELEASE 0x21
#define EVENT_BTN_MID_PRESS     0x30
#define EVENT_BTN_MID_RELEASE   0x31

// ================= VARIÁVEIS GLOBAIS =================
static bool usb_connected = false;
static uint16_t center_x = 2048;
static uint16_t center_y = 2048;
static bool calibrated = false;
static bool btn_left_prev = false;
static bool btn_right_prev = false;
static bool btn_mid_prev = false;

#define EVENT_QUEUE_SIZE 16
typedef struct {
    uint8_t type;
    uint8_t data[3];
    uint8_t len;
} vendor_event_t;

static vendor_event_t event_queue[EVENT_QUEUE_SIZE];
static volatile uint8_t event_head = 0;
static volatile uint8_t event_tail = 0;

// ================= FUNÇÕES AUXILIARES =================
static bool event_push(uint8_t type, const uint8_t *data, uint8_t len) {
    uint8_t next = (event_head + 1) % EVENT_QUEUE_SIZE;
    if (next == event_tail) return false;
    
    event_queue[event_head].type = type;
    event_queue[event_head].len = len > 3 ? 3 : len;
    if (data && len > 0) {
        memcpy(event_queue[event_head].data, data, event_queue[event_head].len);
    }
    event_head = next;
    return true;
}

static bool event_pop(vendor_event_t *out) {
    if (event_tail == event_head) return false;
    *out = event_queue[event_tail];
    event_tail = (event_tail + 1) % EVENT_QUEUE_SIZE;
    return true;
}

// ================= LED RGB =================
void set_rgb_color(uint8_t red, uint8_t green, uint8_t blue) {
    pwm_set_gpio_level(LED_RED_PIN, 255 - red);
    pwm_set_gpio_level(LED_GREEN_PIN, 255 - green);
    pwm_set_gpio_level(LED_BLUE_PIN, 255 - blue);
}

void init_rgb_led(void) {
    gpio_set_function(LED_RED_PIN, GPIO_FUNC_PWM);
    gpio_set_function(LED_GREEN_PIN, GPIO_FUNC_PWM);
    gpio_set_function(LED_BLUE_PIN, GPIO_FUNC_PWM);
    
    pwm_config config = pwm_get_default_config();
    pwm_config_set_clkdiv(&config, 4.0);
    pwm_config_set_wrap(&config, 255);
    
    pwm_init(pwm_gpio_to_slice_num(LED_RED_PIN), &config, true);
    pwm_init(pwm_gpio_to_slice_num(LED_GREEN_PIN), &config, true);
    pwm_init(pwm_gpio_to_slice_num(LED_BLUE_PIN), &config, true);
    
    set_rgb_color(0, 0, 0);
}

void handle_led_command(uint8_t cmd, const uint8_t *data, uint8_t len) {
    switch (cmd) {
        case CMD_LED_OFF:     set_rgb_color(0, 0, 0); break;
        case CMD_LED_RED:     set_rgb_color(255, 0, 0); break;
        case CMD_LED_GREEN:   set_rgb_color(0, 255, 0); break;
        case CMD_LED_BLUE:    set_rgb_color(0, 0, 255); break;
        case CMD_LED_YELLOW:  set_rgb_color(255, 255, 0); break;
        case CMD_LED_CYAN:    set_rgb_color(0, 255, 255); break;
        case CMD_LED_MAGENTA: set_rgb_color(255, 0, 255); break;
        case CMD_LED_WHITE:   set_rgb_color(255, 255, 255); break;
        case CMD_LED_CUSTOM:
            if (len >= 4) {
                set_rgb_color(data[1], data[2], data[3]);
            }
            break;
    }
}

// ================= LED STATUS (ONBOARD) =================
void set_status_led(bool on) {
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, on);
}

void blink_status_led(int times, int delay_ms) {
    for (int i = 0; i < times; i++) {
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
        sleep_ms(delay_ms);
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
        sleep_ms(delay_ms);
    }
}

// ================= CALLBACKS USB =================
void tud_mount_cb(void) { 
    usb_connected = true;
    set_rgb_color(0, 255, 0);
    set_status_led(true);
}

void tud_umount_cb(void) { 
    usb_connected = false;
    set_rgb_color(255, 0, 0);
    set_status_led(false);
}

void tud_vendor_rx_cb(uint8_t itf, uint8_t const* buffer, uint16_t bufsize) {
    (void)itf;
    if (bufsize > 0) {
        handle_led_command(buffer[0], buffer, bufsize);
    }
}

// ================= CALLBACKS HID =================
uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, 
                               hid_report_type_t report_type, 
                               uint8_t* buffer, uint16_t reqlen) {
    (void)instance; (void)report_id; (void)report_type; (void)buffer; (void)reqlen;
    return 0;
}

void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, 
                           hid_report_type_t report_type, 
                           uint8_t const* buffer, uint16_t bufsize) {
    (void)instance; (void)report_id; (void)report_type; (void)buffer; (void)bufsize;
}

// ================= CALIBRAÇÃO =================
void calibrate_joystick(void) {
    if (calibrated) return;
    
    uint32_t sum_x = 0, sum_y = 0;
    const int samples = 50;
    
    blink_status_led(3, 100);
    
    for (int i = 0; i < samples; i++) {
        adc_select_input(0);
        sum_x += adc_read();
        adc_select_input(1);
        sum_y += adc_read();
        sleep_ms(20);
    }
    
    center_x = sum_x / samples;
    center_y = sum_y / samples;
    calibrated = true;
    
    blink_status_led(5, 50);
}

// ================= MOUSE TASK =================
void mouse_task(void) {
    static uint32_t last_read = 0;
    uint32_t now = to_ms_since_boot(get_absolute_time());
    
    if (now - last_read < 1000/POLLING_RATE) return;
    last_read = now;
    
    if (!usb_connected || !tud_hid_ready()) return;
    
    if (!calibrated) {
        calibrate_joystick();
        return;
    }
    
    // Ler joystick
    adc_select_input(0);
    uint16_t x_raw = adc_read();
    adc_select_input(1);
    uint16_t y_raw = adc_read();
    
    int32_t x_diff = (int32_t)x_raw - (int32_t)center_x;
    int32_t y_diff = (int32_t)y_raw - (int32_t)center_y;
    
    int8_t x_move = 0;
    int8_t y_move = 0;
    
    if (x_diff < -DEADZONE) {
        x_move = (x_diff + DEADZONE) / SENSITIVITY;
    } else if (x_diff > DEADZONE) {
        x_move = (x_diff - DEADZONE) / SENSITIVITY;
    }
    
    if (y_diff < -DEADZONE) {
        y_move = (y_diff + DEADZONE) / SENSITIVITY;
    } else if (y_diff > DEADZONE) {
        y_move = (y_diff - DEADZONE) / SENSITIVITY;
    }
    
    if (x_move > MAX_SPEED) x_move = MAX_SPEED;
    if (x_move < -MAX_SPEED) x_move = -MAX_SPEED;
    if (y_move > MAX_SPEED) y_move = MAX_SPEED;
    if (y_move < -MAX_SPEED) y_move = -MAX_SPEED;
    
    // Ler botões
    bool btn_left = !gpio_get(BUTTON_LEFT_PIN);    // Botão A = Esquerdo
    bool btn_right = !gpio_get(BUTTON_RIGHT_PIN);  // Botão B = Direito
    bool btn_mid = !gpio_get(BUTTON_MIDDLE_PIN);   // Joystick = Meio
    
    // Detectar eventos e piscar LED
    if (btn_left && !btn_left_prev) {
        event_push(EVENT_BTN_LEFT_PRESS, NULL, 0);
        // Piscar LED Onboard - Botão Esquerdo
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
        // Flash RGB Vermelho
        set_rgb_color(255, 0, 0);
        sleep_us(50000); // 50ms
        set_rgb_color(0, 255, 0);
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
    } else if (!btn_left && btn_left_prev) {
        event_push(EVENT_BTN_LEFT_RELEASE, NULL, 0);
    }
    
    if (btn_right && !btn_right_prev) {
        event_push(EVENT_BTN_RIGHT_PRESS, NULL, 0);
        // Piscar LED Onboard - Botão Direito
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
        // Flash RGB Azul
        set_rgb_color(0, 0, 255);
        sleep_us(50000); // 50ms
        set_rgb_color(0, 255, 0);
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
    } else if (!btn_right && btn_right_prev) {
        event_push(EVENT_BTN_RIGHT_RELEASE, NULL, 0);
    }
    
    if (btn_mid && !btn_mid_prev) {
        event_push(EVENT_BTN_MID_PRESS, NULL, 0);
        // Piscar LED Onboard - Botão Meio (mais longo)
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
        // Flash RGB Branco
        set_rgb_color(255, 255, 255);
        sleep_us(100000); // 100ms
        set_rgb_color(0, 255, 0);
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
    } else if (!btn_mid && btn_mid_prev) {
        event_push(EVENT_BTN_MID_RELEASE, NULL, 0);
    }
    
    btn_left_prev = btn_left;
    btn_right_prev = btn_right;
    btn_mid_prev = btn_mid;
    
    uint8_t buttons = 0;
    if (btn_left) buttons |= 0x01;  // Botão A = Esquerdo
    if (btn_right) buttons |= 0x02; // Botão B = Direito
    if (btn_mid) buttons |= 0x04;   // Joystick = Meio
    
    uint8_t report[4] = {buttons, (uint8_t)x_move, (uint8_t)y_move, 0};
    tud_hid_report(0, report, sizeof(report));
}

// ================= VENDOR TASK =================
void vendor_task(void) {
    if (!tud_vendor_mounted() || !tud_vendor_write_available()) return;
    
    vendor_event_t event;
    if (event_pop(&event)) {
        uint8_t buf[4];
        buf[0] = event.type;
        memcpy(&buf[1], event.data, event.len);
        
        tud_vendor_write(buf, event.len + 1);
        tud_vendor_flush();
    }
}

// ================= LED HEARTBEAT =================
void heartbeat_task(void) {
    static uint32_t last_blink = 0;
    static bool led_state = false;
    uint32_t now = to_ms_since_boot(get_absolute_time());
    
    if (!usb_connected) {
        if (now - last_blink > 500) {
            led_state = !led_state;
            cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, led_state);
            last_blink = now;
        }
    }
}

// ================= MAIN =================
int main() {
    stdio_init_all();
    
    if (cyw43_arch_init()) {
        while(1) tight_loop_contents();
    }
    
    blink_status_led(3, 200);
    
    init_rgb_led();
    set_rgb_color(255, 0, 0);
    
    adc_init();
    adc_gpio_init(JOYSTICK_X_PIN);
    adc_gpio_init(JOYSTICK_Y_PIN);
    
    gpio_init(BUTTON_LEFT_PIN);
    gpio_set_dir(BUTTON_LEFT_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_LEFT_PIN);
    
    gpio_init(BUTTON_RIGHT_PIN);
    gpio_set_dir(BUTTON_RIGHT_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_RIGHT_PIN);
    
    gpio_init(BUTTON_MIDDLE_PIN);
    gpio_set_dir(BUTTON_MIDDLE_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_MIDDLE_PIN);
    
    tusb_init();
    
    while (true) {
        tud_task();
        mouse_task();
        vendor_task();
        heartbeat_task();
        sleep_ms(1);
    }
    
    return 0;
}