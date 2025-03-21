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

// Allocation page sizes
#define SIZE_4KB (4 * 1024)
#define SIZE_8KB (8 * 1024)
#define SIZE_16KB (16 * 1024)
#define SIZE_32KB (32 * 1024)
#define SIZE_64KB (64 * 1024)
#define SIZE_128KB (128 * 1024)
#define SIZE_256KB (256 * 1024)
#define SIZE_512KB (512 * 1024)
#define SIZE_1MB (1 * 1024 * 1024)
#define SIZE_2MB (2 * 1024 * 1024)
#define SIZE_4MB (4 * 1024 * 1024)
#define SIZE_8MB (8 * 1024 * 1024)
#define SIZE_16MB (16 * 1024 * 1024)

// Allocation page exponents (2^n)
#define EXP_4KB 12
#define EXP_8KB 13
#define EXP_16KB 14
#define EXP_32KB 15
#define EXP_64KB 16
#define EXP_128KB 17
#define EXP_256KB 18
#define EXP_512KB 19
#define EXP_1MB 20
#define EXP_2MB 21
#define EXP_4MB 22
#define EXP_8MB 23
#define EXP_16MB 24

// Macro for storing 2 page sizes in one 64bit value
#define PAGE_SIZES(pg0, pg1) (((uint64_t)(pg0) << 56) | ((uint64_t)(pg1) << 48))

// Hashed Page Table Entry - 128 bits
typedef union
{
	struct
	{
		// part1: 1st 64bits starts here
		uint64_t Reserved : 15; // 0
		uint64_t AVPN : 42;		// Abbreviated Virtual Page Number (vsid | api == avpn)
		uint64_t SW : 4;		// Available for software use
		uint64_t L : 1;			// Large Page Indicator (1=large, 0=4kb)
		uint64_t H : 1;			// Hash Function Identifier
		uint64_t V : 1;			// Valid (1=valid, 0=invalid)

		// part2: 2nd 64bits starts here
		uint64_t Reserved2 : 22; // 0
		uint64_t RPN : 30;		 // Real Page Number
		// uint64_t LP:1;		// Large Page Selector (last bit of RPN if L = '1')
		uint64_t Reserved3 : 2; // 0
		uint64_t AC : 1;		// Address Compare bit
		uint64_t R : 1;			// Reference bit
		uint64_t C : 1;			// Change bit
		uint64_t W : 1;			// Write-through. Hardware always treats this as '0'
		uint64_t I : 1;			// Caching-inhibited bit
		uint64_t M : 1;			// Memory Coherence. Hardware always treats this as '0'
		uint64_t G : 1;			// Guarded bit
		uint64_t N : 1;			// No Execute bit (1=no-execute, 0=allow execute)
		uint64_t PP : 2;		// Page Protection bits
	};
	uint64_t Num[2];
} HTABE;

#define HTABE_ZERO(htabe) memset(&(htabe), 0, sizeof(HTABE))

#define HTABE_IS_VALID(htabe) ((htabe).V != 0)

// *** should this be shifted by 7 or 12 ?
#define HTABE_GET_VA(htabe) (((uint64_t)(htabe).AVPN) << 7)
#define HTABE_SET_VA(htabe, va) ((htabe).AVPN = (uint64_t)(va) >> 7)

// assumes htabe.L has been set with its correct value
#define HTABE_GET_RPN(htabe) ((uint64_t)(htabe).RPN >> ((htabe.L) ? 1 : 0))
#define HTABE_SET_RPN(htabe, rpn) ((htabe).RPN = (uint64_t)(rpn) << ((htabe.L) ? 1 : 0))

// assumes htabe.L has been set with its correct value
#define HTABE_GET_RA(htabe) (((uint64_t)HTABE_GET_RPN(htabe)) << (((htabe).L) ? EXP_16MB : EXP_4KB))
#define HTABE_SET_RA(htabe, ra) HTABE_SET_RPN((htabe), (uint64_t)(ra) >> (((htabe).L) ? EXP_16MB : EXP_4KB))

// va flags
#define HTABE_LARGEPAGE 4
#define HTABE_HASH 2
#define HTABE_VALID 1
#define HTABE_VAMASK 0x1F

// ra flags
#define HTABE_LARGEPAGESEL 0x1000
#define HTABE_ADDRCMP 0x200	 // ?
#define HTABE_ACCESSED 0x100 // R: page referenced
#define HTABE_DIRTY 0x80	 // C: page changed
#define HTABE_WRITETHRU 0x40 // W: cache write-through
#define HTABE_NOCACHE 0x20	 // I: cache inhibit
#define HTABE_COHERENT 0x10	 // M: enforce memory coherence
#define HTABE_GUARDED 0x8
#define HTABE_NOEXEC 0x4
#define HTABE_READONLY 0x3
#define HTABE_READWRITE 0x2
#define HTABE_RAMASK 0x1FFF

// There are 64K HTAB Entries
#define HTAB_COUNT (SIZE_256KB / sizeof(HTABE))

extern void htabe_set_va(HTABE *htabe, uint64_t va, uint64_t flags);
extern void htabe_set_ra(HTABE *htabe, uint64_t ra, uint64_t flags);
extern void htabe_set(HTABE *htabe, uint64_t va, uint64_t vaFlags, uint64_t ra, uint64_t raFlags);
extern void htabe_set_lpar(HTABE *htabe, uint64_t va, uint64_t vaFlags, uint64_t lpar, uint64_t raFlags);

extern uint64_t htab_ra_from_lpar(uint64_t lparAddr);

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

// ESID part of SLB entry as accessed by instructions
typedef union
{
	struct
	{
		uint64_t ESID : 36;	 // Effective Segment ID
		uint64_t V : 1;		 // Valid
		uint64_t Zero : 15;	 // must be set to 0
		uint64_t Index : 12; // Index to select the SLB entry
	};
	uint64_t Num;
} SLBE_ESID;

// VSID part of SLB entry as accessed by instructions
typedef union
{
	struct
	{
		uint64_t VSID : 52; // Virtual Segment ID
		uint64_t Ks : 1;	// Supervisor Key
		uint64_t Kp : 1;	// Problem Key
		uint64_t N : 1;		// No-execute
		uint64_t L : 1;		// Large Page Indicator
		uint64_t C : 1;		// Class
		uint64_t Zero : 7;	// must be set to 0
	};
	uint64_t Num;
} SLBE_VSID;

// SLB entry as accessed by instructions
typedef struct
{
	SLBE_ESID esid;
	SLBE_VSID vsid;
} SLBE;

#define SLBE_VSID_ZERO(vsid) vsid.Num = 0
#define SLBE_ESID_ZERO(esid) esid.Num = 0
#define SLBE_ZERO(slbe) memset(&(slbe), 0, sizeof(SLBE))

#define SLBE_IS_VALID(slbe) ((slbe).esid.V != 0)

#define SLBE_GET_EA(slbe) (((uint64_t)(slbe).esid.ESID) << 28)
#define SLBE_SET_EA(slbe, ea) (slbe).esid.ESID = ((ea) >> 28)
#define ESID_TO_EA(esid) (((uint64_t)(esid)) << 28)
#define EA_TO_ESID(ea) ((ea) >> 28)

#define SLBE_GET_VA(slbe) (((uint64_t)(slbe).vsid.VSID) << 12)
#define SLBE_SET_VA(slbe, va) (slbe).vsid.VSID = ((va) >> 12)
#define VSID_TO_VA(vsid) (((uint64_t)(vsid)) << 12)
#define VA_TO_VSID(va) ((va) >> 12)

#define SLBE_KS 0x800
#define SLBE_KP 0x400
#define SLBE_N 0x200
#define SLBE_L 0x100
#define SLBE_C 0x080

#define SLBE_KERNEL (SLBE_KP | SLBE_C)
#define SLBE_USER (SLBE_KS | SLBE_KP)

#define FLAGS_TO_KS(f) (((f) & SLBE_KS) ? 1 : 0)
#define FLAGS_TO_KP(f) (((f) & SLBE_KP) ? 1 : 0)
#define FLAGS_TO_N(f) (((f) & SLBE_N) ? 1 : 0)
#define FLAGS_TO_L(f) (((f) & SLBE_L) ? 1 : 0)
#define FLAGS_TO_C(f) (((f) & SLBE_C) ? 1 : 0)

// Number of SLB Entries used on the PS3
#define SLB_COUNT 64

// Global copy of the PS3 SLB
// for use with the functions below
extern SLBE SLB[SLB_COUNT];

// calculate kernel vsid value from ea
uint64_t slb_calc_kernel_vsid(uint64_t ea);

// calculate user vsid value from ea and context
uint64_t slb_calc_user_vsid(uint64_t ea, uint64_t context);

// slb[esid.index] = invalid
void slb_invalidate_entry(SLBE_ESID esid);

// slb[esid.index] = vsid|esid
void slb_move_to_entry(SLBE_ESID esid, SLBE_VSID vsid, uint32_t index);

// vsid = slb[index]
void slb_move_from_entry_vsid(SLBE_VSID *vsid, uint32_t index);

// esid = slb[index]
void slb_move_from_entry_esid(SLBE_ESID *esid, uint32_t index);

// This fills the global copy of the PS3 SLB
void slb_read_all_entries(void);

// Adds an entry to the SLB
void slb_add_segment_ex(uint64_t ea, uint64_t va,
						uint32_t Ks, uint32_t Kp, uint32_t N, uint32_t L, uint32_t C);

// Adds an entry to the SLB, specifying flags that are easier
// to understand at a glance.
void slb_add_segment(uint64_t ea, uint64_t va, uint64_t flags);

//

// Special hardcoded addresses used for memory manipulation
#define SPECIAL_EA 0x5000000000000000
#define SPECIAL_VA 0x0000FFFF00000000
#define SPECIAL_VA_MASK 0xFFFFFFFF00000000

// Special flags used for memory manipulation
#define SPECIAL_VA_FLAGS_VALID (HTABE_VALID)
#define SPECIAL_VA_FLAGS_VALID_LARGE (HTABE_VALID | HTABE_LARGEPAGE)
#define SPECIAL_RA_FLAGS_READONLY (HTABE_ACCESSED | HTABE_DIRTY | HTABE_COHERENT | HTABE_NOEXEC | HTABE_READONLY)
#define SPECIAL_RA_FLAGS_READWRITE (HTABE_ACCESSED | HTABE_DIRTY | HTABE_COHERENT | HTABE_NOEXEC | HTABE_READWRITE)

// Macro for creating a special effective address from a base address
// and an index into the HTAB.

#define CALC_SPECIAL_EA(addr, htab_idx, htabe) \
	(uint64_t)((addr) | (((uint64_t)(htab_idx) / 8) ^ ((htabe.AVPN >> 5) & 0x1FFF)) << ((htabe.L) ? EXP_16MB : EXP_4KB))

// Macro for creating a special virtual address from a base address
// and an index into the HTAB.
#define CALC_SPECIAL_VA(addr, idx) \
	((uint64_t)(addr) | (((uint64_t)(idx)) << 16) | ((((((uint64_t)idx) >> 3) ^ (((uint64_t)idx) << 4)) & 0x1800) >> 4))

//

#define PPE_ID0 0
#define PPE_ID1 1

#define PPE_CPU_ID0 0
#define PPE_CPU_ID1 1

//