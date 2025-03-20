#include "include.h"

// ==[ USB 2.0 ]===============================================================

static const struct usb_endpoint_descriptor ep0_out = { // EP0, out to device
    .bLength          = sizeof(struct usb_endpoint_descriptor),
    .bDescriptorType  = USB_DT_ENDPOINT,
    .bEndpointAddress = EP0_OUT_ADDR,
    .bmAttributes     = USB_TRANSFER_TYPE_CONTROL,
    .wMaxPacketSize   = 64,
    .bInterval        = 0
};

static const struct usb_endpoint_descriptor ep0_in = { // EP0, in to host
    .bLength          = sizeof(struct usb_endpoint_descriptor),
    .bDescriptorType  = USB_DT_ENDPOINT,
    .bEndpointAddress = EP0_IN_ADDR,
    .bmAttributes     = USB_TRANSFER_TYPE_CONTROL,
    .wMaxPacketSize   = 64,
    .bInterval        = 0
};

static const struct usb_endpoint_descriptor ep1_out = { // EP1, out to device
    .bLength          = sizeof(struct usb_endpoint_descriptor),
    .bDescriptorType  = USB_DT_ENDPOINT,
    .bEndpointAddress = EP1_OUT_ADDR,
    .bmAttributes     = USB_TRANSFER_TYPE_INTERRUPT,
    .wMaxPacketSize   = 64,
    .bInterval        = 0
};

static const struct usb_endpoint_descriptor ep2_in = { // EP2, in to host
    .bLength          = sizeof(struct usb_endpoint_descriptor),
    .bDescriptorType  = USB_DT_ENDPOINT,
    .bEndpointAddress = EP2_IN_ADDR,
    .bmAttributes     = USB_TRANSFER_TYPE_INTERRUPT,
    .wMaxPacketSize   = 64,
    .bInterval        = 0
};

static const struct usb_interface_descriptor interface_descriptor = {
    .bLength            = sizeof(struct usb_interface_descriptor),
    .bDescriptorType    = USB_DT_INTERFACE,
    .bInterfaceNumber   = 0,    // Starts at zero
    .bAlternateSetting  = 0,    // No alternate
    .bNumEndpoints      = 2,    // Two endpoints (EP0 doesn't count)
    .bInterfaceClass    = 0xff, // Interface class (0xff = Vendor specific)
    .bInterfaceSubClass = 0,    // No subclass
    .bInterfaceProtocol = 0,    // No protocol
    .iInterface         = 5     // String #5
};

static const struct usb_configuration_descriptor config_descriptor = {
    .bLength             = sizeof(struct usb_configuration_descriptor),
    .bDescriptorType     = USB_DT_CONFIG,
    .wTotalLength        = (sizeof(config_descriptor) +
                            sizeof(interface_descriptor) +
                            sizeof(ep1_out) +
                            sizeof(ep2_in)),
    .bNumInterfaces      = 1,    // One interface
    .bConfigurationValue = 1,    // Configuration 1
    .iConfiguration      = 4,    // String #4
    .bmAttributes        = 0xc0, // Attributes: Self-powered, No remote wakeup
    .bMaxPower           = 50    // 100ma (Expressed in 2mA units)
};

static const struct usb_device_descriptor device_descriptor = {
    .bLength            = sizeof(struct usb_device_descriptor),
    .bDescriptorType    = USB_DT_DEVICE,
    .bcdUSB             = 0x0200, // USB 2.0 device
    .bDeviceClass       = 0,      // Defer to interface descriptor
    .bDeviceSubClass    = 0,      // No subclass
    .bDeviceProtocol    = 0,      // No protocol
    .bMaxPacketSize0    = 64,     // Max packet size for EP0
    .idVendor           = 0x2705, // Vendor id
    .idProduct          = 0x0527, // Product id
    .bcdDevice          = 0x0001, // Device release number (xx.yy)
    .iManufacturer      = 1,      // String #1
    .iProduct           = 2,      // String #2
    .iSerialNumber      = 3,      // String #3
    .bNumConfigurations = 1       // One configuration
};

// ==[ PicoUSB ]===============================================================

#define usb_hw_set   ((usb_hw_t *) hw_set_alias_untyped  (usb_hw))
#define usb_hw_clear ((usb_hw_t *) hw_clear_alias_untyped(usb_hw))

typedef void (*usb_ep_handler)(uint8_t *buf, uint16_t len);

void ep0_out_handler(uint8_t *buf, uint16_t len);
void ep0_in_handler (uint8_t *buf, uint16_t len);
void ep1_out_handler(uint8_t *buf, uint16_t len);
void ep2_in_handler (uint8_t *buf, uint16_t len);

struct usb_endpoint {
    const struct usb_endpoint_descriptor *descriptor;
    usb_ep_handler handler;

    volatile uint32_t *endpoint_control;
    volatile uint32_t *buffer_control;
    volatile uint8_t  *data_buffer;

    uint8_t next_datapid; // Toggle DATA0/DATA1 each packet
};

struct usb_device {
    const struct usb_device_descriptor        *device_descriptor;
    const struct usb_configuration_descriptor *config_descriptor;
    const struct usb_interface_descriptor     *interface_descriptor;
    const unsigned char                       *lang_descriptor;
    const unsigned char                       **descriptor_strings;
    struct usb_endpoint                       endpoints[USB_NUM_ENDPOINTS];
};

static const unsigned char lang_descriptor[] = {
    4,         // bLength
    0x03,      // bDescriptorType: string descriptor
    0x09, 0x04 // Language id: US English
};

static const unsigned char *descriptor_strings[] = {
    (unsigned char *) "Kafuu", // String #1: Vendor
    (unsigned char *) "ps3pulldown2"   , // String #2: Product
    (unsigned char *) "12345"  , // String #3: Serial
    (unsigned char *) "Easy"   , // String #4: Configuration
    (unsigned char *) "Simple"   // String #5: Interface
};

static struct usb_device device = {
    .device_descriptor    = &device_descriptor,
    .config_descriptor    = &config_descriptor,
    .interface_descriptor = &interface_descriptor,
    .lang_descriptor      = lang_descriptor,
    .descriptor_strings   = descriptor_strings,
    .endpoints = {
        {
            .descriptor       = &ep0_out,
            .handler          = &ep0_out_handler,
            .endpoint_control = NULL, // EP0 controlled via SIE_CTRL
            .buffer_control   = &usb_dpram->ep_buf_ctrl[0].out,
            .data_buffer      = &usb_dpram->ep0_buf_a[0], // EP0 shares in/out
        },
        {
            .descriptor       = &ep0_in,
            .handler          = &ep0_in_handler,
            .endpoint_control = NULL, //  EP0 controlled via SIE_CTRL
            .buffer_control   = &usb_dpram->ep_buf_ctrl[0].in,
            .data_buffer      = &usb_dpram->ep0_buf_a[0], // EP0 shares in/out
        },
        {
            .descriptor       = &ep1_out,
            .handler          = &ep1_out_handler,
            .endpoint_control = &usb_dpram->ep_ctrl[0].out, // EP1 uses index 0
            .buffer_control   = &usb_dpram->ep_buf_ctrl[1].out,
            .data_buffer      = &usb_dpram->epx_data[0 * 64], // First buffer
        },
        {
            .descriptor       = &ep2_in,
            .handler          = &ep2_in_handler,
            .endpoint_control = &usb_dpram->ep_ctrl[1].in, // EP2 uses index 1
            .buffer_control   = &usb_dpram->ep_buf_ctrl[2].in,
            .data_buffer      = &usb_dpram->epx_data[1 * 64], // Second buffer
        }
    }
};

// Device state management
static uint8_t device_address = 0;
static bool should_set_address = false;
static volatile bool configured = false;

// Buffer for preparing output to send to host
static uint8_t ep0_buf[64];

// ==[ Endpoints ]=============================================================

// Set up an endpoint's control register
void usb_setup_endpoint(const struct usb_endpoint *ep) {

    // Grok the desired endpoint
    uint8_t ep_addr = ep->descriptor->bEndpointAddress;
    uint8_t ep_num = ep_addr & 0x0f;
    bool in = ep_addr & USB_DIR_IN;
    UartPrint("Initialized EP%d_%s (0x%02x) with buffer address 0x%p\n",
           ep_num, in ? "IN " : "OUT", ep_addr, ep->data_buffer);

    // Set ep_ctrl register for this endpoint (skip EP0 since it uses SIE_CTRL)
    if (ep->endpoint_control) {
        uint32_t type = ep->descriptor->bmAttributes << EP_CTRL_BUFFER_TYPE_LSB;
        uint32_t offset = ((uint32_t) ep->data_buffer) ^ ((uint32_t) usb_dpram);
        uint32_t interval = ep->descriptor->bInterval;
        if (interval) {
            interval = (interval - 1) << EP_CTRL_HOST_INTERRUPT_INTERVAL_LSB;
        }
        *ep->endpoint_control = EP_CTRL_ENABLE_BITS          | // Enable EP
                                EP_CTRL_INTERRUPT_PER_BUFFER | // One IRQ per
                                type     | // Control, iso, bulk, or interrupt
                                interval | // Interrupt interval in ms
                                offset   ; // Address base offset in DSPRAM
    }
}

// Set up all endpoints
void usb_setup_endpoints() {
    const struct usb_endpoint *endpoints = device.endpoints;
    for (int i = 0; i < USB_NUM_ENDPOINTS; i++) {
        if (endpoints[i].descriptor && endpoints[i].handler) {
            usb_setup_endpoint(&endpoints[i]);
        }
    }
}

// Get the endpoint for an endpoint address (EP number + direction)
struct usb_endpoint *usb_get_endpoint(uint8_t ep_addr) {
    struct usb_endpoint *endpoints = device.endpoints;
    for (int i = 0; i < USB_NUM_ENDPOINTS; i++) {
        if (endpoints[i].descriptor &&
           (endpoints[i].descriptor->bEndpointAddress == ep_addr)) {
            return &endpoints[i];
        }
    }
    return NULL;
}

// ==[ Transfers ]=============================================================

// Hex dump (mode: 0 = hex; 1 = hex + ascii; 2 = hex + ascii + no newline)
void hexdump(const void* data, size_t size, uint mode) {
    const unsigned char* byte = (const unsigned char *) data;
    size_t i, j;

    for (i = 0; i < size; i += 16) {
        UartPrint("\t| %08zx | ", i); // Print the offset

        // Print hex values
        for (j = 0; j < 16; j++) {
            if (i + j < size) {
                UartPrint("%02x ", byte[i + j]);
            } else {
                UartPrint("   "); // Pad if less than 16 bytes in the line
            }
        }

        UartPrint(" | ");

        if (mode > 1) return;

        // Print ASCII values
        if (mode == 1) {
            for (j = 0; j < 16; j++) {
                if (i + j < size) {
                    unsigned char ch = byte[i + j];
                    UartPrint("%c", (ch >= 32 && ch <= 126) ? ch : '.');
                }
            }
        }

        UartPrint("\n");
    }
}

// Start a transfer on an endpoint
void usb_start_transfer(struct usb_endpoint *ep, uint8_t *buf, uint16_t len) {
    assert(len <= 64);

    // Grok the desired endpoint
    uint8_t ep_addr = ep->descriptor->bEndpointAddress;
    uint8_t ep_num = ep_addr & 0x0f;
    bool in = ep_addr & USB_DIR_IN;

    // Set the buffer control register and copy the buffer if needed
    uint32_t val = len | USB_BUF_CTRL_AVAIL;
    if (in) {
        memcpy((void *) ep->data_buffer, (void *) buf, len);
        val |= USB_BUF_CTRL_FULL; // Mark buffer as full
    }
    val |= ep->next_datapid ? USB_BUF_CTRL_DATA1_PID : USB_BUF_CTRL_DATA0_PID;
    ep->next_datapid ^= 1; // Flip for the next transfer
    *ep->buffer_control = val;
}

// Send a ZLP (zero length packet) to host
void usb_send_zlp() {
    UartPrint("> ZLP\n");
    usb_start_transfer(usb_get_endpoint(EP0_IN_ADDR), NULL, 0);
}

// ==[ Handlers ]==============================================================

void ep0_out_handler(uint8_t *buf, uint16_t len) {
    ; // Nothing to do
    // TODO: Find out when this is called...
}

void ep0_in_handler(uint8_t *buf, uint16_t len) {
    if (should_set_address) {
        usb_hw->dev_addr_ctrl = device_address; // Set hardware device address
        should_set_address = false;
    } else {
        // Prepare for a ZLP from host on EP0_OUT
        usb_start_transfer(usb_get_endpoint(EP0_OUT_ADDR), NULL, 0);
    }
}

#if 0

// Receive a packet from the host on EP1_OUT and send it back to host on EP2_IN
void ep1_out_handler(uint8_t *buf, uint16_t len) {
    usb_start_transfer(usb_get_endpoint(EP2_IN_ADDR), buf, len);
}

// After sending the packet to host, get ready to receive another on EP1_OUT
void ep2_in_handler(uint8_t *buf, uint16_t len) {
    usb_start_transfer(usb_get_endpoint(EP1_OUT_ADDR), NULL, 64);
}

#endif

// ==[ Setup ]=================================================================

// Handle SET_ADDR request from host
void usb_set_device_address(volatile struct usb_setup_packet *pkt) {
    // We can't set the hardware device address yet because we still need to
    // acknowledge the request using a device address of zero. So, we store
    // new the address temporarily and later set it in the ep0_in_handler.
    device_address = (pkt->wValue & 0xff);
    should_set_address = true;
}

// Send device descriptor to host
void usb_send_device_descriptor(volatile struct usb_setup_packet *pkt) {
    const struct usb_device_descriptor *dd = device.device_descriptor;
    uint16_t len = MIN(sizeof(struct usb_device_descriptor), pkt->wLength);
    usb_get_endpoint(EP0_IN_ADDR)->next_datapid = 1; // Reset to DATA1
    usb_start_transfer(usb_get_endpoint(EP0_IN_ADDR), (uint8_t *) dd, len);
}

// Send config descriptor to host
void usb_send_config_descriptor(volatile struct usb_setup_packet *pkt) {
    uint8_t *buf = ep0_buf;

    // Always include the config descriptor
    const struct usb_configuration_descriptor *cd = device.config_descriptor;
    memcpy((void *) buf, cd, sizeof(struct usb_configuration_descriptor));
    buf += sizeof(struct usb_configuration_descriptor);

    // If more than the config descriptor is requested, send everything
    if (pkt->wLength >= cd->wTotalLength) {
        memcpy((void *) buf, device.interface_descriptor,
               sizeof(struct usb_interface_descriptor));
        buf += sizeof(struct usb_interface_descriptor);

        // Copy all the endpoint descriptors starting from EP1
        const struct usb_endpoint *ep = device.endpoints;
        for (uint i = 2; i < USB_NUM_ENDPOINTS; i++) {
            if (ep[i].descriptor) {
                memcpy((void *) buf, ep[i].descriptor,
                       sizeof(struct usb_endpoint_descriptor));
                buf += sizeof(struct usb_endpoint_descriptor);
            }
        }
    }

    // TODO: Fix so we can send more than 64 bytes at a time
    uint32_t len = (uint32_t) buf - (uint32_t) ep0_buf;
    len = MIN(len, pkt->wLength);
    usb_start_transfer(usb_get_endpoint(EP0_IN_ADDR), ep0_buf, len);
}

// Helper to convert a C string to a unicode string descriptor
uint8_t usb_prepare_string_descriptor(const unsigned char *str) {
    uint8_t bLength = 2 + (strlen((const char *) str) * 2);
    uint8_t bDescriptorType = 0x03;
    uint8_t ch;

    volatile uint8_t *buf = ep0_buf;
    *buf++ = bLength;
    *buf++ = bDescriptorType;
    do {
        ch = *str++;
        *buf++ = ch;
        *buf++ = 0;
    } while (ch);

    return bLength;
}

// Send string descriptor to host
void usb_send_string_descriptor(volatile struct usb_setup_packet *pkt) {
    uint8_t i = pkt->wValue & 0xff;
    uint8_t len = 0;

    if (i == 0) {
        len = 4;
        memcpy(ep0_buf, device.lang_descriptor, len);
    } else {
        // Prepare fills in ep0_buf
        len = usb_prepare_string_descriptor(device.descriptor_strings[i - 1]);
    }

    // TODO: Fix so we can send more than 64 bytes at a time
    len = MIN(len, pkt->wLength);
    usb_start_transfer(usb_get_endpoint(EP0_IN_ADDR), ep0_buf, len);
}

// Send configuration value to host
void usb_send_configuration(volatile struct usb_setup_packet *pkt) {
    usb_start_transfer(usb_get_endpoint(EP0_IN_ADDR),
        (uint8_t *) &config_descriptor.bConfigurationValue,
        sizeof(config_descriptor.bConfigurationValue));
}

// Handle SET_CONFIGURATION request from host
void usb_set_device_configuration(volatile struct usb_setup_packet *pkt) {
    configured = true;
}

// Respond to a setup packet from host
void usb_handle_setup_packet() {
    volatile struct usb_setup_packet *pkt =
        (volatile struct usb_setup_packet *) &usb_dpram->setup_packet;
    uint8_t brt = pkt->bmRequestType;
    uint8_t req = pkt->bRequest;

    // Log the behavior
    UartPrint("< Setup");
    hexdump((const void *) pkt, sizeof(struct usb_setup_packet), 2);

    // Force a reset to DATA1 and handle the setup packet
    usb_get_endpoint(EP0_IN_ADDR)->next_datapid = 1;
    if (brt == USB_DIR_OUT) { // Standard device command
        if (req == USB_REQUEST_SET_ADDRESS) {
            UartPrint("Set address to %d\n", (pkt->wValue & 0xff));
            usb_set_device_address(pkt);
        } else if (req == USB_REQUEST_SET_CONFIGURATION) {
            UartPrint("Set configuration to %d\n", (pkt->wValue & 0xff));
            usb_set_device_configuration(pkt);
        } else {
            UartPrint("Unhandled device command\n");
        }
        usb_send_zlp();
    } else if (brt == USB_DIR_IN) { // Standard device request
        if (req == USB_REQUEST_GET_DESCRIPTOR) {
            uint8_t descriptor_type = pkt->wValue >> 8;
            uint8_t index = pkt->wValue & 0xff;

            switch (descriptor_type) {
                case USB_DT_DEVICE:
                    UartPrint("Get device descriptor %d\n", index);
                    usb_send_device_descriptor(pkt);
                    break;
                case USB_DT_CONFIG:
                    UartPrint("Get config descriptor %d\n", index);
                    usb_send_config_descriptor(pkt);
                    break;
                case USB_DT_STRING:
                    UartPrint("Get string descriptor %d\n", index);
                    usb_send_string_descriptor(pkt);
                    break;
                default:
                    UartPrint("Unhandled get descriptor\n");
            }
        } else if (req == USB_REQUEST_GET_CONFIGURATION) {
            UartPrint("Get configuration\n");
            usb_send_configuration(pkt);
        } else {
            UartPrint("Unhandled device request\n");
        }
    } else {
        UartPrint("Unhandled setup packet\n");
    }
}

// ==[ Buffers ]===============================================================

// Notify an endpoint that a transfer has completed
inline static void usb_handle_ep_buff_done(struct usb_endpoint *ep) {
    uint32_t buffer_control = *ep->buffer_control;
    uint16_t len = buffer_control & USB_BUF_CTRL_LEN_MASK; // Get buffer length

#if 0

    // TODO: Logging inside an ISR is bad, fix this soon
    if (len) {
        uint8_t ep_addr = ep->descriptor->bEndpointAddress;
        uint8_t ep_num = ep_addr & 0x0f;
        bool in = ep_addr & USB_DIR_IN;

        // Show the communication details
        UartPrint("%c 0x%02x", in ? '>' : '<', ep_addr);
        hexdump((uint8_t *) ep->data_buffer, len, 1);
    }

#endif

    ep->handler((uint8_t *) ep->data_buffer, len); // Call buffer done handler
}

// Notify the given endpoints that a transfer has completed
static void usb_handle_buff_status() {
    uint32_t buffers = usb_hw->buf_status;
    uint32_t remaining_buffers = buffers;
    uint32_t bit = 1u;
    uint8_t ep_addr;

    for (uint i = 0; remaining_buffers && i < USB_NUM_ENDPOINTS * 2; i++) {
        if (remaining_buffers & bit) {
            usb_hw_clear->buf_status = bit; // Clear this in advance
            ep_addr = (i >> 1u) | (i & 1u ? USB_DIR_OUT : USB_DIR_IN);
            usb_handle_ep_buff_done(usb_get_endpoint(ep_addr));
            remaining_buffers &= ~bit;
        }
        bit <<= 1u;
    }
}

// ==[ Resets ]================================================================

enum {
    USB_SIE_CTRL_BASE = USB_SIE_CTRL_EP0_INT_1BUF_BITS    // Interrupt on every buffer
};

// Reset USB bus
void usb_bus_reset() {
    UartPrint("< Reset\n");
    device_address = 0; // Set address to zero
    usb_hw->dev_addr_ctrl = 0;
    should_set_address = false;
    configured = false;
}

// Reset USB device
void usb_device_reset() {

    // Reset controller
    reset_block       (RESETS_RESET_USBCTRL_BITS);
    unreset_block_wait(RESETS_RESET_USBCTRL_BITS);

    // Clear state
    memset(usb_hw   , 0, sizeof(*usb_hw   ));
    memset(usb_dpram, 0, sizeof(*usb_dpram));

    // Setup endpoints
    usb_setup_endpoints();

    // Configure USB device controller
    usb_hw->muxing    = USB_USB_MUXING_TO_PHY_BITS               | // Connect USB Phy
                        USB_USB_MUXING_SOFTCON_BITS              ; // TODO: What is this?
    usb_hw->pwr       = USB_USB_PWR_VBUS_DETECT_BITS             | // Enable VBUS detection
                        USB_USB_PWR_VBUS_DETECT_OVERRIDE_EN_BITS ; // Enable VBUS detection
    usb_hw->main_ctrl = USB_MAIN_CTRL_CONTROLLER_EN_BITS         ; // Enable controller
    usb_hw->sie_ctrl  = USB_SIE_CTRL_BASE                        ; // Default SIE_CTRL bits
    usb_hw->inte      = USB_INTE_BUS_RESET_BITS                  | // Bus reset
                        USB_INTE_SETUP_REQ_BITS                  | // Setup packet
                        USB_INTE_BUFF_STATUS_BITS                | // Buffer ready
                        USB_INTE_DEV_SUSPEND_BITS                | // Suspend signal
                        USB_INTE_DEV_RESUME_FROM_HOST_BITS       ; // Resume signal

    UartPrint("\nUSB device reset\n\n");
    irq_set_enabled(USBCTRL_IRQ, true);
    usb_hw_set->sie_ctrl = USB_SIE_CTRL_PULLUP_EN_BITS; // "Attach" the device
}

// ==[ Interrupt ]=============================================================

// Interrupt handler
void isr_usbctrl() {
    uint32_t ints = usb_hw->ints;

    // SOF, start of frame

    // Buffer status, one or more buffers have completed
    if (ints &  USB_INTS_BUFF_STATUS_BITS) {
        ints ^= USB_INTS_BUFF_STATUS_BITS;
        usb_handle_buff_status();
    }

    // Setup packet received
    if (ints &  USB_INTS_SETUP_REQ_BITS) {
        ints ^= USB_INTS_SETUP_REQ_BITS;
        usb_hw_clear->sie_status = USB_SIE_STATUS_SETUP_REC_BITS;
        usb_handle_setup_packet();
    }

    // CONN_DIS, device (dis)connected

    // Bus is reset
    if (ints &  USB_INTS_BUS_RESET_BITS) {
        ints ^= USB_INTS_BUS_RESET_BITS;
        usb_hw_clear->sie_status = USB_SIE_STATUS_BUS_RESET_BITS;
        usb_bus_reset();
    }

    // SUSPEND, bus suspended

    if (ints & USB_INTS_DEV_SUSPEND_BITS) {
        ints ^= USB_INTS_DEV_SUSPEND_BITS;
        usb_hw_clear->sie_status = USB_SIE_STATUS_SUSPENDED_BITS;
    }

    // RESUME, bus resumed

    if (ints & USB_INTS_DEV_RESUME_FROM_HOST_BITS) {
        ints ^= USB_INTS_DEV_RESUME_FROM_HOST_BITS;
        usb_hw_clear->sie_status = USB_SIE_STATUS_RESUME_BITS;
    }

    // Any missed?
    if (ints) {
        UartPrint("Unhandled IRQ 0x%08x\n", ints);
    }
}

#if 0

// ==[ Main ]==================================================================

int main() {
    stdio_init_all();
    UartPrint("\033[2J\033[H\n==[ USB device example]==\n\n");
    usb_device_reset();

    // Wait until configured
    while (!configured) { tight_loop_contents(); }
    sleep_ms(500); // brief pause

    UartPrint("\nUSB device configured\n\n");

    // Prepare for up to 64 bytes from host on EP1_OUT
    usb_start_transfer(usb_get_endpoint(EP1_OUT_ADDR), NULL, 64);

    // Everything is interrupt driven so just loop here
    while (1) { tight_loop_contents(); }
}

#endif

bool usb_is_inited()
{
    return configured;
}

void usb_init()
{
    UartPrint("usb_init()\n");

    usb_device_reset();

    while (!usb_is_inited()) {}

    UartPrint("USB device configured\n");

    usb_start_transfer(usb_get_endpoint(EP1_OUT_ADDR), NULL, 64);
}