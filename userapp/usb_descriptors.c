#include "tusb.h"

#define USB_VID 0xCafe
#define USB_PID 0x4002

// Descritor do dispositivo
tusb_desc_device_t const desc_device = {
    .bLength = sizeof(tusb_desc_device_t),
    .bDescriptorType = TUSB_DESC_DEVICE,
    .bcdUSB = 0x0200,
    .bDeviceClass = 0x00,
    .bDeviceSubClass = 0x00,
    .bDeviceProtocol = 0x00,
    .bMaxPacketSize0 = CFG_TUD_ENDPOINT0_SIZE,
    .idVendor = USB_VID,
    .idProduct = USB_PID,
    .bcdDevice = 0x0100,
    .iManufacturer = 0x01,
    .iProduct = 0x02,
    .iSerialNumber = 0x03,
    .bNumConfigurations = 0x01
};

// Report Descriptor para Mouse ABSOLUTO
uint8_t const hid_report_descriptor[] = {
    0x05, 0x01,        // Usage Page (Generic Desktop)
    0x09, 0x02,        // Usage (Mouse)
    0xA1, 0x01,        // Collection (Application)
    
    // Botões (3 botões) - ABSOLUTO
    0x05, 0x09,        //   Usage Page (Button)
    0x19, 0x01,        //   Usage Minimum (1)
    0x29, 0x03,        //   Usage Maximum (3)
    0x15, 0x00,        //   Logical Minimum (0)
    0x25, 0x01,        //   Logical Maximum (1)
    0x95, 0x03,        //   Report Count (3)
    0x75, 0x01,        //   Report Size (1)
    0x81, 0x02,        //   Input (Data, Variable, Absolute)
    
    // Padding
    0x95, 0x01,        //   Report Count (1)
    0x75, 0x05,        //   Report Size (5)
    0x81, 0x01,        //   Input (Constant)
    
    // Eixos X, Y - ABSOLUTO (16 bits cada)
    0x05, 0x01,        //   Usage Page (Generic Desktop)
    0x09, 0x30,        //   Usage (X)
    0x09, 0x31,        //   Usage (Y)
    0x15, 0x00,        //   Logical Minimum (0)
    0x26, 0xFF, 0x7F,  //   Logical Maximum (32767)
    0x35, 0x00,        //   Physical Minimum (0)
    0x46, 0xFF, 0x7F,  //   Physical Maximum (32767)
    0x75, 0x10,        //   Report Size (16 bits)
    0x95, 0x02,        //   Report Count (2)
    0x81, 0x02,        //   Input (Data, Variable, Absolute)
    
    // Wheel (opcional, relativo)
    0x09, 0x38,        //   Usage (Wheel)
    0x15, 0x81,        //   Logical Minimum (-127)
    0x25, 0x7F,        //   Logical Maximum (127)
    0x75, 0x08,        //   Report Size (8)
    0x95, 0x01,        //   Report Count (1)
    0x81, 0x06,        //   Input (Data, Variable, Relative)
    
    0xC0,              // End Collection
    0xC0               // End Collection
};

enum {
    ITF_NUM_HID = 0,
    ITF_NUM_TOTAL
};

#define EPNUM_HID_IN   0x81
#define EPNUM_HID_OUT  0x01

#define CONFIG_TOTAL_LEN (TUD_CONFIG_DESC_LEN + TUD_HID_DESC_LEN)

uint8_t const desc_configuration[] = {
    // Configuração
    TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0,
        CONFIG_TOTAL_LEN, 0x80, 100),
    
    // Interface HID
    TUD_HID_DESCRIPTOR(ITF_NUM_HID, 4, HID_ITF_PROTOCOL_NONE, 
        sizeof(hid_report_descriptor), EPNUM_HID_IN, 64, 10)
};

// Callbacks TinyUSB
uint8_t const *tud_hid_descriptor_report_cb(uint8_t instance) {
    return hid_report_descriptor;
}

uint8_t const *tud_descriptor_device_cb(void) {
    return (uint8_t const *) &desc_device;
}

uint8_t const *tud_descriptor_configuration_cb(uint8_t index) {
    return desc_configuration;
}

// Strings USB
char const *string_desc_arr[] = {
    (const char[]) { 0x09, 0x04 },  // Idioma
    "Pico Mouse Factory",           // Fabricante
    "Pico Mouse Absolute",          // Produto
    "123456",                       // Serial
    "Mouse Interface",              // Interface
};

static uint16_t _desc_str[32];

uint16_t const* tud_descriptor_string_cb(uint8_t index, uint16_t langid) {
    uint8_t chr_count;

    if (index == 0) {
        _desc_str[1] = 0x0409;
        chr_count = 1;
    } else {
        if (index >= sizeof(string_desc_arr)/sizeof(string_desc_arr[0])) {
            return NULL;
        }

        const char* str = string_desc_arr[index];
        chr_count = strlen(str);
        if (chr_count > 31) chr_count = 31;

        for (uint8_t i = 0; i < chr_count; i++) {
            _desc_str[1 + i] = str[i];
        }
    }

    _desc_str[0] = (TUSB_DESC_STRING << 8) | (2 * chr_count + 2);
    return _desc_str;
}