#include "Include.h"

struct GlitcherCtx_s
{
public:
	usbLddOps ldd_ops;

	int32_t our_dev_id;
	bool found;

	bool config_done;

	int32_t pipe_control;

	int32_t pipe_in;
	uint8_t pipe_in_buf[64];

	int32_t pipe_out;
	uint8_t pipe_out_buf[64];

	bool detach_time;

	bool ack_start;
	bool ack_stop;

	bool out_done;
};

GlitcherCtx_s glitcher_ctx;

void Glitcher_callback_in_transfer_done(int32_t result, int32_t count, void *arg)
{
	// PrintLog("Glitcher_callback_in_transfer_done, result = %d, count = %d, arg = 0x%lx\n", result, count, (uint64_t)arg);

	if (result != 0)
	{
		PrintLog("result bad, result = %d\n", result);

		abort();
		return;
	}

	if (count != 64)
	{
		PrintLog("count bad, count = %d\n", count);

		abort();
		return;
	}

	uint8_t v = glitcher_ctx.pipe_in_buf[0];

	// ready to recieve data from device again

	int32_t res;

	res = usbInterruptTransfer(
		glitcher_ctx.pipe_in, glitcher_ctx.pipe_in_buf, 64, Glitcher_callback_in_transfer_done, NULL);

	if (res != 0)
	{
		PrintLog("usbInterruptTransfer failed, res = %d\n", res);

		abort();
		return;
	}

	if (v == 0x11)
		glitcher_ctx.ack_start = true;
	else if (v == 0x22)
		glitcher_ctx.ack_stop = true;
	else
	{
		PrintLog("bad v!, v = %u\n", (uint32_t)v);
	}

	eieio();
}

void Glitcher_callback_out_transfer_done(int32_t result, int32_t count, void *arg)
{
	// PrintLog("Glitcher_callback_out_transfer_done, result = %d, count = %d, arg = 0x%lx\n", result, count, (uint64_t)arg);

	if (result != 0)
	{
		PrintLog("result bad, result = %d\n", result);

		abort();
		return;
	}

	if (count != 64)
	{
		PrintLog("count bad, count = %d\n", count);

		abort();
		return;
	}

	glitcher_ctx.out_done = true;
	eieio();
}

void Glitcher_callback_set_config_done(int32_t result, int32_t count, void *arg)
{
	PrintLog("Glitcher_callback_set_config_done, result = %d, count = %d, arg = 0x%lx\n", result, count, (uint64_t)arg);

	if (result != 0)
	{
		PrintLog("result bad, result = %d\n", result);

		abort();
		return;
	}

	PrintLog("config done.\n");

	// ready to recieve data from device

	int32_t res;

	res = usbInterruptTransfer(
		glitcher_ctx.pipe_in, glitcher_ctx.pipe_in_buf, 64, Glitcher_callback_in_transfer_done, NULL);

	if (res != 0)
	{
		PrintLog("usbInterruptTransfer failed, res = %d\n", res);

		abort();
		return;
	}

	glitcher_ctx.config_done = true;
	eieio();
}

int32_t Glitcher_ldd_ops_probe(int32_t dev_id)
{
	PrintLog("Glitcher_ldd_ops_probe: dev_id = %d\n", dev_id);

	glitcher_ctx.our_dev_id = dev_id;

	PrintLog("Probe success!!\n");
	return USB_PROBE_SUCCEEDED;
}

int32_t Glitcher_ldd_ops_attach(int32_t dev_id)
{
	PrintLog("Glitcher_ldd_ops_attach: dev_id = %d\n", dev_id);

	if (dev_id != glitcher_ctx.our_dev_id)
	{
		PrintLog("Not match our dev id, ignored\n");

		return USB_ATTACH_FAILED;
	}

	bool device_checked = false;

	bool config_checked = false;
	usbConfigDescriptor *config_desc = NULL;

	bool interface_checked = false;

	bool in_endpoint_checked = false;
	bool out_endpoint_checked = false;

	usbEndpointDescriptor *in_endpoint_desc = NULL;
	usbEndpointDescriptor *out_endpoint_desc = NULL;

	uint8_t *desc = NULL;

	while (1)
	{
		desc = (uint8_t *)usbScanStaticDescriptor(dev_id, desc, 0);

		if (desc == NULL)
			break;

		if (desc[1] == USB_DESCRIPTOR_TYPE_DEVICE)
		{
			usbDeviceDescriptor *d = (usbDeviceDescriptor *)desc;

			PrintLog("Checking device...\n");

			PrintLog("bcdUSB = 0x%x\n", (uint32_t)d->bcdUSB);
			PrintLog("bMaxPacketSize0 = 0x%x\n", (uint32_t)d->bMaxPacketSize0);
			PrintLog("idVendor = 0x%x\n", (uint32_t)d->idVendor);
			PrintLog("idProduct = 0x%x\n", (uint32_t)d->idProduct);

			// bcdUSB = 0x2
			// bMaxPacketSize0 = 0x40
			// idVendor = 0x527
			// idProduct = 0x2705

			// vendor and product got swaped, header is wrong

			if (d->bcdUSB != 0x2 || d->bMaxPacketSize0 != 0x40 ||
				d->idProduct != 0x2705 || d->idVendor != 0x0527)
			{
				PrintLog("device check failed\n");
				continue;
			}

			PrintLog("device check ok\n");
			device_checked = true;

			continue;
		}

		if (desc[1] == USB_DESCRIPTOR_TYPE_CONFIG)
		{
			usbConfigDescriptor *d = (usbConfigDescriptor *)desc;

			PrintLog("Checking config...\n");

			if (d->bNumInterfaces != 1)
			{
				PrintLog("config check failed\n");
				continue;
			}

			PrintLog("config check ok\n");

			config_checked = true;
			config_desc = d;

			continue;
		}

		if (desc[1] == USB_DESCRIPTOR_TYPE_INTERFACE)
		{
			usbInterfaceDescriptor *d = (usbInterfaceDescriptor *)desc;

			PrintLog("Checking interface...\n");

			if (d->bNumEndpoints != 2)
			{
				PrintLog("interface check failed\n");
				continue;
			}

			PrintLog("interface check ok\n");
			interface_checked = true;

			continue;
		}

		if (desc[1] == USB_DESCRIPTOR_TYPE_ENDPOINT)
		{
			usbEndpointDescriptor *d = (usbEndpointDescriptor *)desc;

			PrintLog("Checking endpoint...\n");

			uint8_t direction = (d->bEndpointAddress & USB_ENDPOINT_DIRECTION_BITS);
			uint8_t transfer = (d->bmAttributes & USB_ENDPOINT_TRANSFER_TYPE_BITS);

			if (direction == USB_ENDPOINT_DIRECTION_IN)
			{
				if (transfer != USB_ENDPOINT_TRANSFER_TYPE_INTERRUPT)
				{
					PrintLog("in endpoint check failed\n");
					continue;
				}

				PrintLog("in endpoint check ok\n");

				in_endpoint_checked = true;
				in_endpoint_desc = d;

				continue;
			}

			if (direction == USB_ENDPOINT_DIRECTION_OUT)
			{
				if (transfer != USB_ENDPOINT_TRANSFER_TYPE_INTERRUPT)
				{
					PrintLog("out endpoint check failed\n");
					continue;
				}

				PrintLog("out endpoint check ok\n");

				out_endpoint_checked = true;
				out_endpoint_desc = d;

				continue;
			}

			PrintLog("endpoint check failed\n");
			continue;
		}

		PrintLog("Unhandled desc type %d\n", (int32_t)desc[1]);
	}

	if (!device_checked || !config_checked || !interface_checked || !in_endpoint_checked || !out_endpoint_checked)
	{
		PrintLog("Check failed! abort!\n");

		abort();
		return USB_ATTACH_FAILED;
	}

	glitcher_ctx.pipe_control = usbOpenPipe(dev_id, NULL);

	if (glitcher_ctx.pipe_control < 0)
	{
		PrintLog("control usbOpenPipe failed!, res = %d\n", glitcher_ctx.pipe_control < 0);

		abort();
		return USB_ATTACH_FAILED;
	}

	glitcher_ctx.pipe_in = usbOpenPipe(dev_id, in_endpoint_desc);

	if (glitcher_ctx.pipe_in < 0)
	{
		PrintLog("in usbOpenPipe failed!, res = %d\n", glitcher_ctx.pipe_in < 0);

		abort();
		return USB_ATTACH_FAILED;
	}

	glitcher_ctx.pipe_out = usbOpenPipe(dev_id, out_endpoint_desc);

	if (glitcher_ctx.pipe_out < 0)
	{
		PrintLog("in usbOpenPipe failed!, res = %d\n", glitcher_ctx.pipe_out < 0);

		abort();
		return USB_ATTACH_FAILED;
	}

	glitcher_ctx.config_done = false;

	usbSetConfiguration(glitcher_ctx.pipe_control, config_desc->bConfigurationValue,
						Glitcher_callback_set_config_done, NULL);

	while (!glitcher_ctx.config_done)
	{
	}

	PrintLog("Attach success!!\n");
	glitcher_ctx.found = true;
	eieio();

	return USB_ATTACH_SUCCEEDED;
}

int32_t Glitcher_ldd_ops_detach(int32_t dev_id)
{
	PrintLog("Glitcher_ldd_ops_detach: dev_id = %d\n", dev_id);

	if (!glitcher_ctx.detach_time || !glitcher_ctx.found)
	{
		PrintLog("Not detach time! abort!\n");
		abort();

		return USB_DETACH_FAILED;
	}

	int32_t res;

	res = usbClosePipe(glitcher_ctx.pipe_out);

	if (res != 0)
	{
		PrintLog("out usbClosePipe failed!, res = %d\n", res);

		abort();
		return USB_ATTACH_FAILED;
	}

	res = usbClosePipe(glitcher_ctx.pipe_in);

	if (res != 0)
	{
		PrintLog("in usbClosePipe failed!, res = %d\n", res);

		abort();
		return USB_ATTACH_FAILED;
	}

	res = usbClosePipe(glitcher_ctx.pipe_control);

	if (res != 0)
	{
		PrintLog("control usbClosePipe failed!, res = %d\n", res);

		abort();
		return USB_ATTACH_FAILED;
	}

	glitcher_ctx.found = false;

	PrintLog("Detach success!!\n");
	return USB_DETACH_SUCCEEDED;
}

void Glitcher_Init()
{
	PrintLog("Glitcher_Init()\n");

	int32_t res;

	res = sysModuleLoad(SYSMODULE_USB);

	if (res != 0)
	{
		PrintLog("sysModuleLoad failed!, res = %d\n", res);

		abort();
		return;
	}

	res = usbInit();

	if (res != 0)
	{
		PrintLog("usbInit failed!, res = %d\n", res);

		abort();
		return;
	}

	//

	glitcher_ctx.ldd_ops.name = "desc";

	glitcher_ctx.ldd_ops.probe = Glitcher_ldd_ops_probe;

	glitcher_ctx.ldd_ops.attach = Glitcher_ldd_ops_attach;
	glitcher_ctx.ldd_ops.detach = Glitcher_ldd_ops_detach;

	//

	glitcher_ctx.detach_time = false;
	glitcher_ctx.found = false;

	//

	res = usbRegisterExtraLdd(&glitcher_ctx.ldd_ops, 0x2705, 0x0527);

	if (res != 0)
	{
		PrintLog("usbRegisterExtraLdd failed!, res = %d\n", res);

		abort();
		return;
	}

	while (!glitcher_ctx.found)
	{
	}

	if (!glitcher_ctx.found)
	{
		PrintLog("Glitcher not found!, abort!\n");

		abort();
		return;
	}

	//

	WaitInMs(500);

	PrintLog("Glitcher_Init() done\n");
}

void Glitcher_Destroy()
{
	PrintLog("Glitcher_Destroy()\n");

	// can never detach
	glitcher_ctx.detach_time = false;

	int32_t res;

	res = usbUnregisterExtraLdd(&glitcher_ctx.ldd_ops);

	if (res != 0)
	{
		PrintLog("usbUnregisterExtraLdd failed!, res = %d\n", res);

		abort();
		return;
	}

#if 0

    while (glitcher_ctx.found) {};

    if (glitcher_ctx.found)
    {
        PrintLog("Glitcher not detach!, abort!\n");

        abort();
        return;
    }

#endif

	res = usbEnd();

	if (res != 0)
	{
		PrintLog("usbEnd failed!, res = %d\n", res);

		abort();
		return;
	}

	res = sysModuleUnload(SYSMODULE_USB);

	if (res != 0)
	{
		PrintLog("sysModuleUnload failed!, res = %d\n", res);

		abort();
		return;
	}

	PrintLog("Glitcher_Destroy() done\n");
}

void Glitcher_Start()
{
	// PrintLog("Glitcher_Start()\n");

	glitcher_ctx.out_done = false;
	glitcher_ctx.ack_start = false;

	glitcher_ctx.pipe_out_buf[0] = 0x44;

	eieio();

	int32_t res;

	// uint64_t t1 = GetTimeInUs();

	res = usbInterruptTransfer(
		glitcher_ctx.pipe_out, glitcher_ctx.pipe_out_buf, 64, Glitcher_callback_out_transfer_done, NULL);

	if (res != 0)
	{
		PrintLog("usbInterruptTransfer failed, res = %d\n", res);

		abort();
		return;
	}

	while (!glitcher_ctx.out_done)
	{
	}
	// uint64_t t2 = GetTimeInUs();
	while (!glitcher_ctx.ack_start)
	{
	}

	// uint64_t t3 = GetTimeInUs();

	// PrintLog("Glitcher_Start() done. delta1 = %lu us, delta2 = %lu us\n", (t2 - t1), (t3 - t2));
}

void Glitcher_Stop()
{
	// PrintLog("Glitcher_Stop()\n");

	glitcher_ctx.out_done = false;
	glitcher_ctx.ack_stop = false;

	glitcher_ctx.pipe_out_buf[0] = 0x55;

	eieio();

	int32_t res;

	// uint64_t t1 = GetTimeInUs();

	res = usbInterruptTransfer(
		glitcher_ctx.pipe_out, glitcher_ctx.pipe_out_buf, 64, Glitcher_callback_out_transfer_done, NULL);

	if (res != 0)
	{
		PrintLog("usbInterruptTransfer failed, res = %d\n", res);

		abort();
		return;
	}

	while (!glitcher_ctx.out_done)
	{
	}
	// uint64_t t2 = GetTimeInUs();
	while (!glitcher_ctx.ack_stop)
	{
	}

	// uint64_t t3 = GetTimeInUs();

	// PrintLog("Glitcher_Stop() done. delta1 = %lu us, delta2 = %lu us\n", (t2 - t1), (t3 - t2));
}