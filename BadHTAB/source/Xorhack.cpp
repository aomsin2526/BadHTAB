//
// Hashed Page Table (HTAB) code for PS3
// Based on the info from:
//   PowerPC Architecture, Book III v2.02
//   Cell Broadband Engine Handbook v1.0
//
// HTAB (VA to RA mapping)
// Used to map one Virtual Page Number (VPN) to its currently assigned
// Real Page Number (RPN). So it specifies a mapping between VA and RA.
// Since the PPE only supports 42bit RAs, the max HTAB size is 16MB.
//
// xorloser - February 2010
// www.xorloser.com
//

#include "Include.h"

void htabe_set_va(HTABE *htabe, uint64_t va, uint64_t flags)
{
	htabe->Num[0] = flags & HTABE_VAMASK;
	HTABE_SET_VA(*htabe, va);
}

void htabe_set_ra(HTABE *htabe, uint64_t ra, uint64_t flags)
{
	htabe->Num[1] = flags & HTABE_RAMASK;
	HTABE_SET_RA(*htabe, ra);
}

void htabe_set(HTABE *htabe, uint64_t va, uint64_t vaFlags, uint64_t ra, uint64_t raFlags)
{
	htabe->Num[0] = vaFlags & HTABE_VAMASK;
	HTABE_SET_VA(*htabe, va);

	htabe->Num[1] = raFlags & HTABE_RAMASK;
	HTABE_SET_RA(*htabe, ra);
}

void htabe_set_lpar(HTABE *htabe, uint64_t va, uint64_t vaFlags, uint64_t lpar, uint64_t raFlags)
{
	htabe->Num[0] = vaFlags & HTABE_VAMASK;
	HTABE_SET_VA(*htabe, va);

	htabe->Num[1] = lpar | (raFlags & HTABE_RAMASK);
}

uint64_t htab_ra_from_lpar(uint64_t lparAddr)
{
	int32_t res;

	uint64_t lpar_start_addr;

	res = lv1_query_logical_partition_address_region_info(lparAddr, &lpar_start_addr, NULL, NULL, NULL, NULL);

	if (res != 0)
	{
		PrintLog("lv1_query_logical_partition_address_region_info failed!!!, res = %d\n", res);
		abort();
		return 0;
	}

	HTABE htabe;
	htabe_set_lpar(&htabe,
				   0x0001408F92C94400ULL, HTABE_VALID,
				   lpar_start_addr, HTABE_ACCESSED | HTABE_DIRTY | HTABE_COHERENT | HTABE_NOEXEC | HTABE_READONLY);

	uint32_t htab_idx = FindFreeHTABIdx();

	res = lv1_write_htab_entry(0, htab_idx, 0, 0);

	if (res != 0)
	{
		PrintLog("lv1_write_htab_entry failed!!!, res = %d\n", res);
		abort();
		return 0;
	}

	res = lv1_write_htab_entry(0, htab_idx, htabe.Num[0], htabe.Num[1]);

	if (res != 0)
	{
		PrintLog("lv1_write_htab_entry failed!!!, res = %d\n", res);
		abort();
		return 0;
	}

	lv2_read(CalcGameOSHTAB_EA_Addr_By_HtabIdx(htab_idx), sizeof(HTABE), &htabe);

	uint64_t ra_addr = HTABE_GET_RA(htabe);
	uint64_t result_addr = ra_addr + (lparAddr - lpar_start_addr);

	res = lv1_write_htab_entry(0, htab_idx, 0, 0);

	if (res != 0)
	{
		PrintLog("lv1_write_htab_entry failed!!!, res = %d\n", res);
		abort();
		return 0;
	}

	return result_addr;
}

//

//
// Segment Lookaside Buffer (SLB) code for PS3
// Based on the info from:
//   PowerPC Architecture, Book III v2.02
//   Cell Broadband Engine Handbook v1.0
//
// SLB (EA to VA mapping)
// Segment Lookaside Buffer (SLB) has two unified (instruction and data)
// caches, one per PPE thread. Each cache has 64 entries and is used to
// provide EA-to-VA translations. The PPE suports 2^37 segments of 256MB
// each. Segments are protected areas of virtual memory. Segments can be
// selected as execute or no-execute. The SLB is maintained with special
// instructions. Each 256MB segment can hold 64K pages of 4KB each (or
// fewer of larger page sizes). The pages that make up a segment can
// overlap, but they must do so completely, not partially.
//
// The small or flat segmentation model is the simplest. It uses a single
// segment for all instructions, data, and stacks. In this model, virtual
// address translation is, in effect, implemented entirely with paging.
//
// Multiple segments can be allocated for use by the instructions, private
// data, and stack of each process. They can also be allocated for shared
// libraries, shared memory-mapped I/O ports, and so forth. Access to
// segments can be protected by access-type and privilege attributes,
// so processes and data can be shared without interference from
// unauthorised programs.
//
// xorloser - February 2010
// www.xorloser.com
//

// Global copy of the PS3 SLB
// for use with the functions below
SLBE SLB[SLB_COUNT];

// calculate kernel vsid value from ea
uint64_t slb_calc_kernel_vsid(uint64_t ea)
{
	uint64_t proto_vsid = EA_TO_ESID(ea);
	return (proto_vsid * 0xBF6E61BULL) % 0xFFFFFFFFFULL;
}

// calculate user vsid value from ea and context
uint64_t slb_calc_user_vsid(uint64_t ea, uint64_t context)
{
	uint64_t proto_vsid = (context << 16) | EA_TO_ESID(ea);
	return (proto_vsid * 0xBF6E61BULL) % 0xFFFFFFFFFULL;
}

// slb[esid.index] = invalid
void slb_invalidate_entry(SLBE_ESID esid)
{
	esid.Zero = 0;
	esid.Index = 0;

	// asm volatile(	"slbie %0\n"
	//				:
	//				: "r" (esid) );

	lv2_slbie(esid.Num);
}

// slb[esid.index] = vsid|esid
void slb_move_to_entry(SLBE_ESID esid, SLBE_VSID vsid, uint32_t index)
{
	esid.Index = index;

	// asm volatile(	"slbmte %0, %1\n"
	//				:
	//				: "r" (vsid), "r" (esid) );

	lv2_slbmte(vsid.Num, esid.Num);
}

// vsid = slb[index]
void slb_move_from_entry_vsid(SLBE_VSID *vsid, uint32_t index)
{
	SLBE_ESID slb_index;
	SLBE_ESID_ZERO(slb_index);
	slb_index.Index = index;

	// asm volatile(	"slbmfev %0, %1\n"
	//				: "=r" (*vsid)
	//				: "r" (slb_index) );

	lv2_slbmfev(slb_index.Num, &vsid->Num);
}

// esid = slb[index]
void slb_move_from_entry_esid(SLBE_ESID *esid, uint32_t index)
{
	SLBE_ESID slb_index;
	SLBE_ESID_ZERO(slb_index);
	slb_index.Index = index;

	// asm volatile(	"slbmfee %0, %1\n"
	//				: "=r" (*esid)
	//				: "r" (slb_index) );

	lv2_slbmfee(slb_index.Num, &esid->Num);

	esid->Index = index;
}

// This fills the global copy of the PS3 SLB
void slb_read_all_entries(void)
{
	int slb_idx;
	for (slb_idx = 0; slb_idx < SLB_COUNT; slb_idx++)
	{
		slb_move_from_entry_esid(&SLB[slb_idx].esid, slb_idx);
		slb_move_from_entry_vsid(&SLB[slb_idx].vsid, slb_idx);
	}
}

// Adds an entry to the SLB
void slb_add_segment_ex(uint64_t ea, uint64_t va,
						uint32_t Ks, uint32_t Kp, uint32_t N, uint32_t L, uint32_t C)
{
	SLBE slbe;
	int slb_idx;

	// invalidate any existing SLB entry that is the same as
	// the entry being added to ensure there is only ever one
	// entry for the values being added.
	SLBE_ZERO(slbe);
	slbe.esid.ESID = EA_TO_ESID(ea);
	slb_invalidate_entry(slbe.esid);

	// find the first unused (invalid) SLB entry
	// and then add this new SLB entry there
	slb_read_all_entries();
	for (slb_idx = 0; slb_idx < SLB_COUNT; slb_idx++)
	{
		if (!SLBE_IS_VALID(SLB[slb_idx]))
		{
			// setup the ESID values for the segment to add
			SLBE_ZERO(slbe);
			slbe.esid.V = 1;
			slbe.esid.Index = slb_idx;
			slbe.esid.ESID = EA_TO_ESID(ea);

			// setup the VSID values for the segment to add
			slbe.vsid.Ks = Ks;
			slbe.vsid.Kp = Kp;
			slbe.vsid.N = N;
			slbe.vsid.L = L;
			slbe.vsid.C = C;
			slbe.vsid.VSID = VA_TO_VSID(va);

			slb_move_to_entry(slbe.esid, slbe.vsid, slb_idx);
			return;
		}
	}
}

// Adds an entry to the SLB, specifying flags that are easier
// to understand at a glance.
void slb_add_segment(uint64_t ea, uint64_t va, uint64_t flags)
{
	slb_add_segment_ex(ea, va, FLAGS_TO_KS(flags), FLAGS_TO_KP(flags),
					   FLAGS_TO_N(flags), FLAGS_TO_L(flags), FLAGS_TO_C(flags));
}