#ifndef _USB_COMMON_H
#define _USB_COMMON_H

#include "pico/types.h"
#include "hardware/structs/usb.h"

#define USB_REQ_TYPE_STANDARD            0x00u
#define USB_REQ_TYPE_TYPE_CLASS          0x20u
#define USB_REQ_TYPE_TYPE_VENDOR         0x40u
#define USB_REQ_TYPE_TYPE_MASK           0x60u

#define USB_REQ_TYPE_RECIPIENT_DEVICE    0x00u
#define USB_REQ_TYPE_RECIPIENT_INTERFACE 0x01u
#define USB_REQ_TYPE_RECIPIENT_ENDPOINT  0x02u
#define USB_REQ_TYPE_RECIPIENT_MASK      0x1fu

#define USB_DIR_OUT                      0x00u
#define USB_DIR_IN                       0x80u

#define USB_TRANSFER_TYPE_CONTROL        0x00u
#define USB_TRANSFER_TYPE_ISOCHRONOUS    0x01u
#define USB_TRANSFER_TYPE_BULK           0x02u
#define USB_TRANSFER_TYPE_INTERRUPT      0x03u
#define USB_TRANSFER_TYPE_BITS           0x03u

#define USB_DT_DEVICE                    0x01u
#define USB_DT_CONFIG                    0x02u
#define USB_DT_STRING                    0x03u
#define USB_DT_INTERFACE                 0x04u
#define USB_DT_ENDPOINT                  0x05u

#define USB_REQUEST_GET_STATUS           0x00u
#define USB_REQUEST_CLEAR_FEATURE        0x01u
#define USB_REQUEST_SET_FEATURE          0x03u
#define USB_REQUEST_SET_ADDRESS          0x05u
#define USB_REQUEST_GET_DESCRIPTOR       0x06u
#define USB_REQUEST_SET_DESCRIPTOR       0x07u
#define USB_REQUEST_GET_CONFIGURATION    0x08u
#define USB_REQUEST_SET_CONFIGURATION    0x09u
#define USB_REQUEST_GET_INTERFACE        0x0au
#define USB_REQUEST_SET_INTERFACE        0x0bu
#define USB_REQUEST_SYNC_FRAME           0x0cu

#define USB_REQUEST_MSC_GET_MAX_LUN      0xfeu
#define USB_REQUEST_MSC_RESET            0xffu

#define USB_FEAT_ENDPOINT_HALT           0x00u
#define USB_FEAT_DEVICE_REMOTE_WAKEUP    0x01u
#define USB_FEAT_TEST_MODE               0x02u

struct usb_descriptor {
    uint8_t  bLength;
    uint8_t  bDescriptorType;
} __packed;

struct usb_device_descriptor {
    uint8_t  bLength;
    uint8_t  bDescriptorType;
    uint16_t bcdUSB;
    uint8_t  bDeviceClass;
    uint8_t  bDeviceSubClass;
    uint8_t  bDeviceProtocol;
    uint8_t  bMaxPacketSize0;
    uint16_t idVendor;
    uint16_t idProduct;
    uint16_t bcdDevice;
    uint8_t  iManufacturer;
    uint8_t  iProduct;
    uint8_t  iSerialNumber;
    uint8_t  bNumConfigurations;
} __packed;

struct usb_configuration_descriptor {
    uint8_t  bLength;
    uint8_t  bDescriptorType;
    uint16_t wTotalLength;
    uint8_t  bNumInterfaces;
    uint8_t  bConfigurationValue;
    uint8_t  iConfiguration;
    uint8_t  bmAttributes;
    uint8_t  bMaxPower;
} __packed;

struct usb_interface_descriptor {
    uint8_t  bLength;
    uint8_t  bDescriptorType;
    uint8_t  bInterfaceNumber;
    uint8_t  bAlternateSetting;
    uint8_t  bNumEndpoints;
    uint8_t  bInterfaceClass;
    uint8_t  bInterfaceSubClass;
    uint8_t  bInterfaceProtocol;
    uint8_t  iInterface;
} __packed;

struct usb_endpoint_descriptor {
    uint8_t  bLength;
    uint8_t  bDescriptorType;
    uint8_t  bEndpointAddress;
    uint8_t  bmAttributes;
    uint16_t wMaxPacketSize;
    uint8_t  bInterval;
} __packed;

struct usb_endpoint_descriptor_long {
    uint8_t  bLength;
    uint8_t  bDescriptorType;
    uint8_t  bEndpointAddress;
    uint8_t  bmAttributes;
    uint16_t wMaxPacketSize;
    uint8_t  bInterval;
    uint8_t  bRefresh;
    uint8_t  bSyncAddr;
} __packed;

struct usb_setup_packet {
    uint8_t  bmRequestType;
    uint8_t  bRequest;
    uint16_t wValue;
    uint16_t wIndex;
    uint16_t wLength;
} __packed;

typedef struct usb_descriptor               usb_descriptor_t;
typedef struct usb_device_descriptor        usb_device_descriptor_t;
typedef struct usb_configuration_descriptor usb_configuration_descriptor_t;
typedef struct usb_interface_descriptor     usb_interface_descriptor_t;
typedef struct usb_endpoint_descriptor      usb_endpoint_descriptor_t;
typedef struct usb_endpoint_descriptor_long usb_endpoint_descriptor_long_t;
typedef struct usb_setup_packet             usb_setup_packet_t;

//

#define EP0_OUT_ADDR (USB_DIR_OUT | 0)
#define EP0_IN_ADDR  (USB_DIR_IN  | 0)
#define EP1_OUT_ADDR (USB_DIR_OUT | 1)
#define EP2_IN_ADDR  (USB_DIR_IN  | 2)

struct usb_endpoint *usb_get_endpoint(uint8_t ep_addr);

void usb_start_transfer(struct usb_endpoint *ep, uint8_t *buf, uint16_t len);

bool usb_is_inited();

void usb_init();

#endif
