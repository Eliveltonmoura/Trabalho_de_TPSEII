#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/usb.h>
#include <linux/mutex.h>
#include <linux/slab.h>

#define VID 0xCAFE
#define PID 0x4003
#define DEVICE_NAME "pico_mouse"

/* Protocol Commands */
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

/* Device state */
struct pico_mouse_dev {
    struct usb_device *udev;
    struct usb_interface *interface;
    unsigned char ep_in;
    unsigned char ep_out;
    struct mutex io_mutex;
    unsigned long packets_sent;
    unsigned long packets_received;
    unsigned long errors;
};

static struct pico_mouse_dev *pico_device = NULL;

/* -------------------------------------------------------
 *                     OPEN / RELEASE
 * -------------------------------------------------------*/
static int pico_mouse_open(struct inode *inode, struct file *file)
{
    struct pico_mouse_dev *dev = pico_device;
    
    if (!dev || !dev->udev) {
        pr_err("pico_mouse: device not initialized\n");
        return -ENODEV;
    }

    file->private_data = dev;
    pr_info("pico_mouse: device opened (packets: sent=%lu, recv=%lu, errors=%lu)\n",
            dev->packets_sent, dev->packets_received, dev->errors);
    return 0;
}

static int pico_mouse_release(struct inode *inode, struct file *file)
{
    pr_info("pico_mouse: device closed\n");
    return 0;
}

/* -------------------------------------------------------
 *                     WRITE (Host → Device)
 * -------------------------------------------------------*/
static ssize_t pico_mouse_write(struct file *file, const char __user *user_buf,
                                size_t count, loff_t *ppos)
{
    struct pico_mouse_dev *dev = file->private_data;
    unsigned char *kbuf;
    int ret, actual;
    
    if (!dev || !dev->udev) {
        pr_err("pico_mouse: device not connected\n");
        return -ENODEV;
    }

    if (count > 64)
        count = 64;

    kbuf = kmalloc(count, GFP_KERNEL);
    if (!kbuf)
        return -ENOMEM;

    if (copy_from_user(kbuf, user_buf, count)) {
        pr_err("pico_mouse: copy_from_user failed\n");
        ret = -EFAULT;
        goto exit_write;
    }

    mutex_lock(&dev->io_mutex);
    ret = usb_bulk_msg(dev->udev,
                       usb_sndbulkpipe(dev->udev, dev->ep_out),
                       kbuf, count, &actual, 2000);
    mutex_unlock(&dev->io_mutex);

    if (ret) {
        pr_err("pico_mouse: usb_bulk_msg write failed: %d\n", ret);
        dev->errors++;
        goto exit_write;
    }

    dev->packets_sent++;
    pr_debug("pico_mouse: wrote %d bytes (cmd=0x%02x)\n", actual, kbuf[0]);
    ret = actual;

exit_write:
    kfree(kbuf);
    return ret;
}

/* -------------------------------------------------------
 *                     READ (Device → Host)
 * -------------------------------------------------------*/
static ssize_t pico_mouse_read(struct file *file, char __user *user_buf,
                               size_t count, loff_t *ppos)
{
    struct pico_mouse_dev *dev = file->private_data;
    unsigned char *kbuf;
    int ret, actual;
    
    if (!dev || !dev->udev) {
        pr_err("pico_mouse: device not connected\n");
        return -ENODEV;
    }

    if (count > 64)
        count = 64;

    kbuf = kmalloc(count, GFP_KERNEL);
    if (!kbuf)
        return -ENOMEM;

    mutex_lock(&dev->io_mutex);
    ret = usb_bulk_msg(dev->udev,
                       usb_rcvbulkpipe(dev->udev, dev->ep_in),
                       kbuf, count, &actual, 2000);
    mutex_unlock(&dev->io_mutex);

    if (ret) {
        pr_err("pico_mouse: usb_bulk_msg read failed: %d\n", ret);
        dev->errors++;
        goto exit_read;
    }

    if (copy_to_user(user_buf, kbuf, actual)) {
        pr_err("pico_mouse: copy_to_user failed\n");
        ret = -EFAULT;
        goto exit_read;
    }

    dev->packets_received++;
    pr_debug("pico_mouse: read %d bytes (event=0x%02x)\n", actual, kbuf[0]);
    ret = actual;

exit_read:
    kfree(kbuf);
    return ret;
}

/* -------------------------------------------------------
 *                     FILE OPERATIONS
 * -------------------------------------------------------*/
static const struct file_operations pico_mouse_fops = {
    .owner = THIS_MODULE,
    .open = pico_mouse_open,
    .release = pico_mouse_release,
    .write = pico_mouse_write,
    .read = pico_mouse_read,
};

static struct usb_class_driver pico_mouse_class = {
    .name = "pico_mouse%d",
    .fops = &pico_mouse_fops,
    .minor_base = 200,
};

/* -------------------------------------------------------
 *                     PROBE / DISCONNECT
 * -------------------------------------------------------*/
static int pico_mouse_probe(struct usb_interface *interface,
                            const struct usb_device_id *id)
{
    struct pico_mouse_dev *dev;
    struct usb_host_interface *iface_desc;
    struct usb_endpoint_descriptor *endpoint;
    int i, ret;

    dev = kzalloc(sizeof(*dev), GFP_KERNEL);
    if (!dev)
        return -ENOMEM;

    dev->udev = usb_get_dev(interface_to_usbdev(interface));
    dev->interface = interface;
    mutex_init(&dev->io_mutex);

    /* Get the vendor interface (interface 1) */
    iface_desc = interface->cur_altsetting;
    
    /* Find bulk endpoints */
    for (i = 0; i < iface_desc->desc.bNumEndpoints; i++) {
        endpoint = &iface_desc->endpoint[i].desc;
        
        if (usb_endpoint_is_bulk_in(endpoint)) {
            dev->ep_in = endpoint->bEndpointAddress;
            pr_info("pico_mouse: EP IN = 0x%02x\n", dev->ep_in);
        }
        
        if (usb_endpoint_is_bulk_out(endpoint)) {
            dev->ep_out = endpoint->bEndpointAddress;
            pr_info("pico_mouse: EP OUT = 0x%02x\n", dev->ep_out);
        }
    }

    if (!dev->ep_in || !dev->ep_out) {
        pr_err("pico_mouse: could not find endpoints\n");
        ret = -ENODEV;
        goto error;
    }

    /* Save device pointer */
    usb_set_intfdata(interface, dev);
    pico_device = dev;

    /* Register device */
    ret = usb_register_dev(interface, &pico_mouse_class);
    if (ret) {
        pr_err("pico_mouse: failed to register device\n");
        pico_device = NULL;
        goto error;
    }

    pr_info("pico_mouse: Pico Mouse connected on /dev/pico_mouse%d\n",
            interface->minor);
    return 0;

error:
    kfree(dev);
    return ret;
}

static void pico_mouse_disconnect(struct usb_interface *interface)
{
    struct pico_mouse_dev *dev = usb_get_intfdata(interface);

    if (dev) {
        pr_info("pico_mouse: disconnecting (stats: sent=%lu, recv=%lu, errors=%lu)\n",
                dev->packets_sent, dev->packets_received, dev->errors);
        
        usb_deregister_dev(interface, &pico_mouse_class);
        pico_device = NULL;
        usb_set_intfdata(interface, NULL);
        kfree(dev);
    }
}

/* -------------------------------------------------------
 *                     USB DRIVER
 * -------------------------------------------------------*/
static const struct usb_device_id pico_mouse_id_table[] = {
    { USB_DEVICE_INTERFACE_NUMBER(VID, PID, 1) }, // Interface 1 (Vendor)
    {}
};
MODULE_DEVICE_TABLE(usb, pico_mouse_id_table);

static struct usb_driver pico_mouse_driver = {
    .name = "pico_mouse",
    .probe = pico_mouse_probe,
    .disconnect = pico_mouse_disconnect,
    .id_table = pico_mouse_id_table,
};

module_usb_driver(pico_mouse_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("TPSE2 Lab");
MODULE_DESCRIPTION("USB Driver for Pico Mouse Joystick with HID+Vendor interfaces");
MODULE_VERSION("1.0");