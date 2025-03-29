extern uint64_t FindHvcallTable();

extern bool IsOurHvcallInstalled();

extern void InstallOurHvcall();
extern void UninstallOurHvcall();

struct CallLv1Exec_Context_s
{
public:
	uint64_t ra;
	uint64_t args[7];

	uint64_t out[8];
};

extern void CallLv1Exec(CallLv1Exec_Context_s* ctx);

struct CallLv1ExecEa_Context_s
{
public:
	uint64_t ea;
	uint64_t size; // size of function

	uint64_t args[7];

	uint64_t out[8];
};

extern void CallLv1ExecEa(CallLv1ExecEa_Context_s* ctx);

extern "C"
{
	extern void our_lv1_exec_test_do();
	extern uint64_t our_lv1_exec_test_do_size;

	extern void our_lv1_test_puts_do();
	extern uint64_t our_lv1_test_puts_do_size;

	extern void our_lv1_auth_lv2_hook_fself_do();
	extern uint64_t our_lv1_auth_lv2_hook_fself_do_size;
};

extern void lv1_test_puts();