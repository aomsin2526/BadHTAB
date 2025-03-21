#define MM_EA2VA(ea) ((ea) & ~(1ULL << 63))

extern int32_t mm_insert_htab_entry(uint64_t vas_id, uint64_t va_addr, uint64_t lpar_addr,
									uint64_t page_shift, uint64_t vflags, uint64_t rflags, uint64_t *index);

extern int32_t mm_map_lpar_memory_region(uint64_t vas_id, uint64_t va_start_addr, uint64_t lpar_start_addr,
										 uint64_t size, uint64_t page_shift, uint64_t vflags, uint64_t rflags);

extern void map_lpar_to_lv2_ea(uint64_t lpar_addr, uint64_t ea_addr, uint64_t size, bool readonly, bool exec);

extern void map_physical_to_lv2_ea(uint64_t phys_addr, uint64_t ea_addr, uint64_t size, uint64_t *out_lpar_addr);
extern void unmap_physical_to_lv2_ea(uint64_t lpar_addr);