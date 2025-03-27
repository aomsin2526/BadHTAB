.global our_lv1_peek_do

// in: r3 = addr
// out: r3 = value

our_lv1_peek_do:
	ld %r3, 0(%r3)
	blr

.global our_lv1_poke_do

// in: r3 = addr, r4 = value
// out: r3 = 0

our_lv1_poke_do:
	std %r4, 0(%r3)
	li %r3, 0
	blr

// in: r3-r8 = args, r9 = addr

.global our_lv1_exec_do

our_lv1_exec_do:

	// push
	addi %r1, %r1, -16

	mflr %r0
	std %r0, 0(%r1)

	mtctr %r9
	bctrl

	ld %r0, 0(%r1)
	mtlr %r0

	// pop
	addi %r1, %r1, 16

	blr

// test
// in: r9 = start addr of this function

.global our_lv1_exec_test_do

our_lv1_exec_test_do:
	li %r3, 0x39
	blr

.global our_lv1_exec_test_do_size

our_lv1_exec_test_do_size:
	.quad (our_lv1_exec_test_do_size - our_lv1_exec_test_do)

// test puts
// in: r9 = start addr of this function

// puts = 0x2BA460

.global our_lv1_test_puts_do

our_lv1_test_puts_do:

	// push
	addi %r1, %r1, -512

	mflr %r0
	std %r0, 0(%r1)

	ld %r0, (our_lv1_test_puts_do_puts_addr - our_lv1_test_puts_do)(%r9)
	mtctr %r0

	mr %r3, %r9
	addi %r3, %r3, (our_lv1_test_puts_do_test_str - our_lv1_test_puts_do)

	bctrl

	ld %r0, 0(%r1)
	mtlr %r0

	// pop
	addi %r1, %r1, 512

	blr

our_lv1_test_puts_do_test_str:
	.asciz "Hello from lv1_puts\n"

.align 4

our_lv1_test_puts_do_puts_addr:
	.quad 0x2BA460

.global our_lv1_test_puts_do_size

our_lv1_test_puts_do_size:
	.quad (our_lv1_test_puts_do_size - our_lv1_test_puts_do)