#include "Include.h"

void LoadLv2Kernel()
{
	PrintLog("LoadLv2Kernel()\n");

	if (!IsFileExist("/dev_flash/lv2_kernel.self"))
	{
		PrintLog("/dev_flash/lv2_kernel.self not found! Abort!()\n");

		abort();
		return;
	}

	bool found = false;
	uint64_t offset = 0;

	if (fwVersion == 4.84)
	{
		offset = 0x1480C8;
		found = true;
	}
	else if (fwVersion == 4.92)
	{
		offset = 0x376E78;
		found = true;
	}
	else
	{
		uint64_t t1 = GetTimeInMs();

		for (uint64_t i = 0; i < 16 * 1024 * 1024; i += 4)
		{
			uint64_t v;
			lv1_read(i, 8, &v);

			if (v == 0x2F6F732F6C76325FULL)
			{
				found = true;
				offset = i - 4;

				break;
			}

			uint64_t t2 = GetTimeInMs();

			if ((t2 - t1) >= 2000)
			{
				lv2_beep_single();
				t1 = t2;
			}
		}
	}

	if (!found)
	{
		PrintLog("offset not found!, Abort!()\n");

		abort();
		return;
	}

	PrintLog("offset = 0x%lx\n", offset);

	{
		uint64_t patches[4];

		patches[0] = 0x2F6C6F63616C5F73ULL;
		patches[1] = 0x7973302F6C76325FULL;
		patches[2] = 0x6B65726E656C2E73ULL;
		patches[3] = 0x656C660000000000ULL;

		lv1_write(offset, 32, patches);
	}

	WaitInMs(1000);
	lv2_beep_triple();

	PrintLog("Booting lv2_kernel...\n");
	lv2_boot_lv2_kernel();
}