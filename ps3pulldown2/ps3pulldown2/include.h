//#define UART_ENABLED 1

#define PULLDOWN1_ENABLED 1
#define PULLDOWN2_ENABLED 1

// no usb, always glitching
//#define TEST_MODE_ENABLED 1

//#define GLITCH_CORE_ENABLED 1

#define SHUFFLE_ENABLED 1

#include <stdio.h>
#include <string.h>

#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "hardware/irq.h"
#include "hardware/clocks.h"
#include "hardware/vreg.h"
#include "hardware/regs/usb.h" // USB hardware registers from pico-sdk
#include "hardware/structs/usb.h" // USB hardware structs from pico-sdk
#include "hardware/resets.h" // For resetting the native USB controller

#include "pico/multicore.h"
#include "pico/rand.h"

#include "usb_common.h"

#define UART_ID uart0
#define BAUD_RATE 115200
#define DATA_BITS 8
#define STOP_BITS 1
#define PARITY    UART_PARITY_NONE

#define UART_TX_PIN 12

#if UART_ENABLED
extern char uartBuf[8192];
#define UartPrint(...) { sprintf(uartBuf, __VA_ARGS__); uart_puts(UART_ID, uartBuf); }
#else
#define UartPrint(...) (true)
#endif