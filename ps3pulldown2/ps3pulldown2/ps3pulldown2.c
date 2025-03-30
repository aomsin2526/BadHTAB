#include "include.h"

static const int led_pin_id = PICO_DEFAULT_LED_PIN;

static const int pulldown1_pin_id = 15;
static const int pulldown2_pin_id = 16;

volatile bool cur_led_status = false;

volatile bool do_glitch = false;

volatile bool is_stopped = false;

char uartBuf[8192];

void WaitInUs(uint32_t us)
{
	// sleep_us(us);
	busy_wait_us(us);
}

void UartInit()
{
	uart_init(UART_ID, 2400);

	gpio_set_function(UART_TX_PIN, UART_FUNCSEL_NUM(UART_ID, UART_TX_PIN));

	uart_set_baudrate(UART_ID, BAUD_RATE);

	uart_set_hw_flow(UART_ID, false, false);

	uart_set_format(UART_ID, DATA_BITS, STOP_BITS, PARITY);

	uart_set_fifo_enabled(UART_ID, false);
}

void ep1_out_handler(uint8_t *buf, uint16_t len)
{
	uint8_t v = buf[0];
	uint8_t response = 0;

	usb_start_transfer(usb_get_endpoint(EP1_OUT_ADDR), NULL, 64);

	if (v == 0x44)
	{
		response = 0x11;

		{
			uint8_t newbuf[64];
			newbuf[0] = response;

			usb_start_transfer(usb_get_endpoint(EP2_IN_ADDR), newbuf, 64);
		}

		// led
		cur_led_status = !cur_led_status;
		gpio_put(led_pin_id, cur_led_status);

		do_glitch = true;
		__dsb();
	}
	else if (v == 0x45) // no ack needed
	{
		// led
		cur_led_status = !cur_led_status;
		gpio_put(led_pin_id, cur_led_status);

		do_glitch = true;
		__dsb();
	}
	else if (v == 0x55)
	{
		is_stopped = false;
		__dsb();
		do_glitch = false;
		__dsb();

#if GLITCH_CORE_ENABLED
		while (!is_stopped)
		{
		}
#endif

		// led
		cur_led_status = !cur_led_status;
		gpio_put(led_pin_id, cur_led_status);

		response = 0x22;

		{
			uint8_t newbuf[64];
			newbuf[0] = response;

			usb_start_transfer(usb_get_endpoint(EP2_IN_ADDR), newbuf, 64);
		}
	}
}

void ep2_in_handler(uint8_t *buf, uint16_t len)
{
}

void glitch_core()
{
	uint32_t randValue = get_rand_32();

	bool shuffle = (randValue % 2) == 1;

	while (1)
	{
		if (do_glitch)
		{
			WaitInUs(1000);
			//for (uint32_t i = 0; i < 1; i++)
			while (1)
			{
				uint32_t randValue = get_rand_32();

				uint32_t waitValueInUs = (randValue % 1000);
				WaitInUs(1000 + waitValueInUs);

				if (!do_glitch)
					break;

#if PULLDOWN1_ENABLED

#if PULLDOWN1_ENABLED && PULLDOWN2_ENABLED && SHUFFLE_ENABLED
				if (!shuffle)
#endif
				{
#if !GLITCH_CORE_ENABLED && !TEST_MODE_ENABLED
					irq_set_enabled(USBCTRL_IRQ, false);
#endif
					// pull down for 40ns

					gpio_set_dir(pulldown1_pin_id, GPIO_OUT);
					// gpio_set_drive_strength(pulldown1_pin_id, GPIO_DRIVE_STRENGTH_12MA);

					gpio_put(pulldown1_pin_id, false);

					// pull down
					gpio_set_function(pulldown1_pin_id, GPIO_FUNC_SIO);

					// then we float it
					io_bank0_hw->io[pulldown1_pin_id].ctrl = GPIO_FUNC_NULL << IO_BANK0_GPIO0_CTRL_FUNCSEL_LSB;

#if !GLITCH_CORE_ENABLED && !TEST_MODE_ENABLED
					irq_set_enabled(USBCTRL_IRQ, true);
#endif
				}

#endif

#if PULLDOWN2_ENABLED

#if PULLDOWN1_ENABLED && PULLDOWN2_ENABLED && SHUFFLE_ENABLED
				if (shuffle)
#endif
				{
#if !GLITCH_CORE_ENABLED && !TEST_MODE_ENABLED
					irq_set_enabled(USBCTRL_IRQ, false);
#endif

					// pull down for 40ns

					gpio_set_dir(pulldown2_pin_id, GPIO_OUT);
					// gpio_set_drive_strength(pulldown2_pin_id, GPIO_DRIVE_STRENGTH_12MA);

					gpio_put(pulldown2_pin_id, false);

					// pull down
					gpio_set_function(pulldown2_pin_id, GPIO_FUNC_SIO);

					// then we float it
					io_bank0_hw->io[pulldown2_pin_id].ctrl = GPIO_FUNC_NULL << IO_BANK0_GPIO0_CTRL_FUNCSEL_LSB;

#if !GLITCH_CORE_ENABLED && !TEST_MODE_ENABLED
					irq_set_enabled(USBCTRL_IRQ, true);
#endif
				}

#endif

				shuffle = !shuffle;
			}

			while (do_glitch) {};
		}

		if (!is_stopped)
		{
			is_stopped = true;
			__dsb();
		}
	}
}

void main()
{
	// init clock

	vreg_set_voltage(VREG_VOLTAGE_1_30);
	sleep_ms(200);

	// set_sys_clock_khz(24000, true);
	// set_sys_clock_khz(50000, true);

	//set_sys_clock_khz(100000, true); // 120 - 140ns, 160ns on second pico
	//set_sys_clock_khz(200000, true); // 70ns
	set_sys_clock_khz(250000, true); // most stable?, 70ns on second pico
	//set_sys_clock_khz(300000, true); // 35 - 40ns
	//set_sys_clock_khz(320000, true);

	// deinit pulldown

	gpio_deinit(pulldown1_pin_id);
	gpio_deinit(pulldown2_pin_id);

	// init uart

#if UART_ENABLED
	UartInit();
#endif

	//

	UartPrint("UART ready.\n");

#if !TEST_MODE_ENABLED
	// init usb
	usb_init();
#endif

	// init led gpio

	gpio_init(led_pin_id);
	gpio_set_dir(led_pin_id, GPIO_OUT);

	cur_led_status = true;
	gpio_put(led_pin_id, cur_led_status);

	//

	UartPrint("Ready.\n");

	//

#if TEST_MODE_ENABLED
	do_glitch = true;
#endif

#if GLITCH_CORE_ENABLED
	multicore_launch_core1(glitch_core);
#else
	glitch_core();
#endif

	while (1)
	{
	}
}