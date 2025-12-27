#include "tusb.h"

#define USB_VID 0xCafe
#define USB_PID 0x4003

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

uint8_t const hid_report_descriptor[] = {
    HID_USAGE_PAGE ( HID_USAGE_PAGE_DESKTOP     ),
    HID_USAGE      ( HID_USAGE_DESKTOP_MOUSE    ),
    HID_COLLECTION ( HID_COLLECTION_APPLICATION ),
        HID_USAGE_PAGE  ( HID_USAGE_PAGE_BUTTON ),
        HID_USAGE_MIN   ( 1 ),
        HID_USAGE_MAX   ( 3 ),
        HID_LOGICAL_MIN ( 0 ),
        HID_LOGICAL_MAX ( 1 ),
        HID_REPORT_COUNT( 3 ),
        HID_REPORT_SIZE ( 1 ),
        HID_INPUT       ( HID_DATA | HID_VARIABLE | HID_ABSOLUTE ),
        HID_REPORT_COUNT( 1 ),
        HID_REPORT_SIZE ( 5 ),
        HID_INPUT       ( HID_CONSTANT ),
        HID_USAGE_PAGE  ( HID_USAGE_PAGE_DESKTOP ),
        HID_USAGE       ( HID_USAGE_DESKTOP_X ),
        HID_USAGE       ( HID_USAGE_DESKTOP_Y ),
        HID_LOGICAL_MIN ( 0x81 ),
        HID_LOGICAL_MAX ( 0x7f ),
        HID_REPORT_COUNT( 2 ),
        HID_REPORT_SIZE ( 8 ),
        HID_INPUT       ( HID_DATA | HID_VARIABLE | HID_RELATIVE ),
        HID_USAGE       ( HID_USAGE_DESKTOP_WHEEL ),
        HID_LOGICAL_MIN ( 0x81 ),
        HID_LOGICAL_MAX ( 0x7f ),
        HID_REPORT_COUNT( 1 ),
        HID_REPORT_SIZE ( 8 ),
        HID_INPUT       ( HID_DATA | HID_VARIABLE | HID_RELATIVE ),
    HID_COLLECTION_END
};

enum {
    ITF_NUM_HID = 0,
    ITF_NUM_VENDOR,
    ITF_NUM_TOTAL
};

#define EPNUM_HID_IN      0x81
#define EPNUM_VENDOR_OUT  0x02
#define EPNUM_VENDOR_IN   0x82
#define CONFIG_TOTAL_LEN  (TUD_CONFIG_DESC_LEN + TUD_HID_DESC_LEN + TUD_VENDOR_DESC_LEN)

uint8_t const desc_configuration[] = {
    TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, CONFIG_TOTAL_LEN, 
                         TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 100),
    TUD_HID_DESCRIPTOR(ITF_NUM_HID, 4, HID_ITF_PROTOCOL_MOUSE, 
                      sizeof(hid_report_descriptor), EPNUM_HID_IN, 64, 10),
    TUD_VENDOR_DESCRIPTOR(ITF_NUM_VENDOR, 5, EPNUM_VENDOR_OUT, EPNUM_VENDOR_IN, 64)
};

char const *string_desc_arr[] = {
    (const char[]) { 0x09, 0x04 },
    "TPSE2 Lab",
    "Pico Mouse Joystick Composite",
    "123456",
    "Mouse HID Interface",
    "LED Control Interface",
};

static uint16_t _desc_str[32];

uint8_t const *tud_descriptor_device_cb(void) {
    return (uint8_t const *)&desc_device;
}

uint8_t const *tud_descriptor_configuration_cb(uint8_t index) {
    (void)index;
    return desc_configuration;
}

uint8_t const *tud_hid_descriptor_report_cb(uint8_t instance) {
    (void)instance;
    return hid_report_descriptor;
}

uint16_t const *tud_descriptor_string_cb(uint8_t index, uint16_t langid) {
    (void)langid;
    uint8_t chr_count;

    if (index == 0) {
        memcpy(&_desc_str[1], string_desc_arr[0], 2);
        chr_count = 1;
    } else {
        if (index >= sizeof(string_desc_arr) / sizeof(string_desc_arr[0])) {
            return NULL;
        }
        const char *str = string_desc_arr[index];
        chr_count = (uint8_t)strlen(str);
        if (chr_count > 31) chr_count = 31;
        for (uint8_t i = 0; i < chr_count; i++) {
            _desc_str[1 + i] = str[i];
        }
    }
    _desc_str[0] = (uint16_t)((TUSB_DESC_STRING << 8) | (2 * chr_count + 2));
    return _desc_str;
}