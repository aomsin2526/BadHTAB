#include "Include.h"

#define SYS_TIMEBASE_GET(tb)                          \
	do                                                \
	{                                                 \
		__asm__ volatile("1: mftb %[current_tb];"     \
						 "cmpwi 7, %[current_tb], 0;" \
						 "beq-  7, 1b;"               \
						 : [current_tb] "=r"(tb) :    \
						 : "cr7");                    \
	} while (0)

uint64_t my_timebase = sysGetTimebaseFrequency();

uint64_t GetTimeInNs()
{
	uint64_t cur_tb_ns;
	SYS_TIMEBASE_GET(cur_tb_ns);

	cur_tb_ns *= MUL_NS;

	return (cur_tb_ns / my_timebase);
}

void WaitInNs(uint64_t ns)
{
	uint64_t start = GetTimeInNs();
	uint64_t end = start + ns;

	while (1)
	{
		uint64_t cur = GetTimeInNs();

		if (cur >= end)
			return;
	}
}

uint64_t GetTimeInUs()
{
	uint64_t cur_tb_us;
	SYS_TIMEBASE_GET(cur_tb_us);

	cur_tb_us *= MUL_US;

	return (cur_tb_us / my_timebase);
}

void WaitInUs(uint64_t us)
{
	uint64_t start = GetTimeInUs();
	uint64_t end = start + us;

	while (1)
	{
		uint64_t cur = GetTimeInUs();

		if (cur >= end)
			return;
	}
}

uint64_t GetTimeInMs()
{
	uint64_t cur_tb_ms;
	SYS_TIMEBASE_GET(cur_tb_ms);

	cur_tb_ms *= MUL_MS;

	return (cur_tb_ms / my_timebase);
}

void WaitInMs(uint64_t ms)
{
	uint64_t start = GetTimeInMs();
	uint64_t end = start + ms;

	while (1)
	{
		uint64_t cur = GetTimeInMs();

		if (cur >= end)
			return;
	}
}

void Sleep(uint32_t secs)
{
	sysSleep(secs);
}

bool IsFileExist(const char* path)
{
	FILE* f = fopen(path, "rb");

	if (f == NULL)
		return false;

	fclose(f);
	return true;
}

bool FlashIsNor()
{
	uint8_t flag;

	int32_t res = lv2_storage_get_cache_of_flash_ext_flag(&flag);

	if (res != 0)
	{
		PrintLog("lv2_storage_get_cache_of_flash_ext_flag failed!, res = %d\n", res);

		abort();
		return false;
	}

	return !(flag & 0x1);
}

bool TargetIsCEX()
{
	uint64_t type;

	int32_t res = lv2_dbg_get_console_type(&type);

	if (res != 0)
	{
		PrintLog("lv2_dbg_get_console_type failed!, res = %d\n", res);

		abort();
		return false;
	}

	return (type == 1);
}

bool TargetIsDEX()
{
	uint64_t type;

	int32_t res = lv2_dbg_get_console_type(&type);

	if (res != 0)
	{
		PrintLog("lv2_dbg_get_console_type failed!, res = %d\n", res);

		abort();
		return false;
	}

	return (type == 2);
}

bool TargetIsDECR()
{
	uint64_t type;

	int32_t res = lv2_dbg_get_console_type(&type);

	if (res != 0)
	{
		PrintLog("lv2_dbg_get_console_type failed!, res = %d\n", res);

		abort();
		return false;
	}

	return (type == 3);
}