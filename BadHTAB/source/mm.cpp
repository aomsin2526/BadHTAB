#include "Include.h"

#define HTAB_HASH_MASK 0x7FFULL
#define HPTES_PER_GROUP 8

#define HPTE_V_AVPN_SHIFT 7
#define HPTE_V_BOLTED 0x0000000000000010ULL
#define HPTE_V_LARGE 0x0000000000000004ULL
#define HPTE_V_VALID 0x0000000000000001ULL
#define HPTE_V_FLAGS_MASK 0x000000000000007FULL

#define HPTE_R_R 0x0000000000000100ULL
#define HPTE_R_C 0x0000000000000080ULL
#define HPTE_R_W 0x0000000000000040ULL
#define HPTE_R_I 0x0000000000000020ULL
#define HPTE_R_M 0x0000000000000010ULL
#define HPTE_R_G 0x0000000000000008ULL
#define HPTE_R_N 0x0000000000000004ULL
#define HPTE_R_FLAGS_MASK 0x000000000000FFFFULL

// #define PAGE_SIZES(_ps0, _ps1)				(((uint64_t) (_ps0) << 56) | ((uint64_t) (_ps1) << 48))
#define PAGE_SIZE_4KB 12
#define PAGE_SIZE_64KB 16
#define PAGE_SIZE_1MB 20
#define PAGE_SIZE_16MB 24

int32_t mm_insert_htab_entry(uint64_t vas_id, uint64_t va_addr, uint64_t lpar_addr,
							 uint64_t page_shift, uint64_t vflags, uint64_t rflags, uint64_t *index)
{
	uint64_t hpte_group, hpte_index, hpte_v, hpte_r, hpte_evicted_v, hpte_evicted_r;
	int32_t result;

	hpte_group = (((va_addr >> 28) ^ ((va_addr & 0xFFFFFFFULL) >> page_shift)) & HTAB_HASH_MASK) *
				 HPTES_PER_GROUP;

	hpte_v = ((va_addr >> 23) << HPTE_V_AVPN_SHIFT) | HPTE_V_VALID | (vflags & HPTE_V_FLAGS_MASK);

	if (page_shift != PAGE_SIZE_4KB)
		hpte_v |= HPTE_V_LARGE;

	hpte_r = (lpar_addr & ~((1ULL << page_shift) - 1)) | (rflags & HPTE_R_FLAGS_MASK);

	if (page_shift == PAGE_SIZE_1MB)
		hpte_r |= (1 << 12);

	result = lv1_insert_htab_entry(vas_id, hpte_group, hpte_v, hpte_r, HPTE_V_BOLTED, 0,
								   &hpte_index, &hpte_evicted_v, &hpte_evicted_r);

	if (result != 0)
		return result;

	if (index != 0)
		*index = hpte_index;

	return 0;
}

int32_t mm_map_lpar_memory_region(uint64_t vas_id, uint64_t va_start_addr, uint64_t lpar_start_addr,
								  uint64_t size, uint64_t page_shift, uint64_t vflags, uint64_t rflags)
{
	int32_t result;

	uint64_t i;

	for (i = 0; i < (size >> page_shift); i++)
	{
		result = mm_insert_htab_entry(vas_id, va_start_addr, lpar_start_addr,
									  page_shift, vflags, rflags, 0);

		if (result != 0)
			return result;

		va_start_addr += (1 << page_shift);
		lpar_start_addr += (1 << page_shift);
	}

	return 0;
}

void map_lpar_to_lv2_ea(uint64_t lpar_addr, uint64_t ea_addr, uint64_t size, bool readonly, bool exec)
{
	int32_t res = -9999;

	uint64_t page_size = PAGE_SIZE_4KB;

	uint64_t rflags = readonly ? SPECIAL_RA_FLAGS_READONLY : SPECIAL_RA_FLAGS_READWRITE;

	if (exec)
		rflags &= ~HTABE_NOEXEC;
	else
		rflags |= HTABE_NOEXEC;

	res = mm_map_lpar_memory_region(0, MM_EA2VA(ea_addr), lpar_addr, size, page_size, 0, rflags);

	if (res != 0)
	{
		PrintLog("mm_map_lpar_memory_region failed! res = %d\n", res);
		abort();
	}
}

void map_physical_to_lv2_ea(uint64_t phys_addr, uint64_t ea_addr, uint64_t size, uint64_t *out_lpar_addr)
{
	int32_t res = -9999;

	uint64_t page_size = PAGE_SIZE_4KB;

	uint64_t lpar_addr;

	res = lv1_map_physical_address_region(phys_addr, page_size, size, &lpar_addr);

	if (res != 0)
	{
		PrintLog("lv1_map_physical_address_region failed! res = %d\n", res);
		abort();
	}

	res = mm_map_lpar_memory_region(0, MM_EA2VA(ea_addr), lpar_addr, size, page_size, 0, 0);

	if (res != 0)
	{
		PrintLog("mm_map_lpar_memory_region failed! res = %d\n", res);
		abort();
	}

	if (out_lpar_addr != NULL)
		*out_lpar_addr = lpar_addr;
}

void unmap_physical_to_lv2_ea(uint64_t lpar_addr)
{
	int32_t res = -9999;

	res = lv1_unmap_physical_address_region(lpar_addr);

	if (res != 0)
	{
		PrintLog("lv1_unmap_physical_address_region failed! res = %d\n", res);
		abort();
	}
}