extern uint64_t lv2_peek(uint64_t addr);
extern void lv2_poke(uint64_t addr, uint64_t val);

extern void lv2_read(uint64_t addr, uint64_t size, void *out_Buf);
extern void lv2_write(uint64_t addr, uint64_t size, const void *in_Buf);

extern uint64_t lv2_lv1_peek(uint64_t addr);
extern void lv2_lv1_poke(uint64_t addr, uint64_t val);

extern void lv2_lv1_read(uint64_t addr, uint64_t size, void *out_Buf);
extern void lv2_lv1_write(uint64_t addr, uint64_t size, const void *in_Buf);

extern void lv2_beep_single();
extern void lv2_beep_triple();
extern void lv2_beep_long();

struct CallLv2Function_Context_s
{
	uint64_t addr; // function address
	uint64_t args[7];

	uint64_t out[8];
};

extern void CallLv2Function(CallLv2Function_Context_s *ctx);

extern "C"
{
	void lv2_mfsprg0_do();

	void lv2_slbie_do();

	void lv2_slbmte_do();

	void lv2_slbmfev_do();
	void lv2_slbmfee_do();

	void lv2_disable_interrupt_do();
	void lv2_enable_interrupt_do();

	void lv2_disable_enable_interrupt_do();

	void lv2_write_lv1_rw_htabe_do();

	void lv2_get_pir_do();

	void lv2_glitcher_test_do();

	void lv2_lv1_release_memory_intr_do();
};

extern uint64_t lv2_mfsprg0();

extern void lv2_slbie(uint64_t esid);

extern void lv2_slbmte(uint64_t vsid, uint64_t esid);

extern void lv2_slbmfev(uint64_t slb_index, uint64_t *out_vsid);
extern void lv2_slbmfee(uint64_t slb_index, uint64_t *out_esid);

extern void lv2_disable_interrupt();
extern void lv2_enable_interrupt();

extern void lv2_disable_enable_interrupt();

extern void lv2_write_lv1_rw_htabe(uint64_t old_vas_id, uint64_t new_vas_id);

extern void lv2_get_pir(uint64_t *out_pir);

extern void lv2_glitcher_test(uint64_t addr, uint64_t size, uint64_t *out_WriteCount);

extern int32_t lv2_lv1_release_memory_intr(uint64_t lpar_addr);