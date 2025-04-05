.global lv2_mfsprg0_do

lv2_mfsprg0_do:
    mfsprg0 4
    blr

.global lv2_slbie_do

lv2_slbie_do:
    slbie 3
    blr

.global lv2_slbmte_do

lv2_slbmte_do:
    slbmte 3, 4
    blr

.global lv2_slbmfev_do

lv2_slbmfev_do:
    slbmfev 4, 3
    blr

.global lv2_slbmfee_do

lv2_slbmfee_do:
    slbmfee 4, 3
    blr

.global lv2_disable_interrupt_do

lv2_disable_interrupt_do:
    li      %r0, 2
    mtmsrd  %r0, 1
    eieio
    sync
	blr

.global lv2_enable_interrupt_do

lv2_enable_interrupt_do:
    li  %r0, 0
    ori %r0, %r0, 0x8002
    eieio
    sync
    mtmsrd  %r0, 1
    blr

.global lv2_disable_enable_interrupt_do

lv2_disable_enable_interrupt_do:
    // disable
    li %r0, 2
    mtmsrd  %r0, 1

    //enable
    li %r0, 0
    ori %r0, %r0, 0x8002
    eieio
    sync
    mtmsrd  %r0, 1

    blr

.global lv2_write_lv1_rw_htabe_do

// in
// (r4 = old_vas_id, r5 = new_vas_id)

// out
// (r4 = res1, r5 = res2)

lv2_write_lv1_rw_htabe_do:

    // disable interrupt
    li %r0, 2
    mtmsrd  %r0, 1
    eieio
    sync

    // offsets:
    // 0 = old_vas_id
    // 8 = new_vas_id
    // 16 = res1
    // 24 = res2
    // 32 = r0 restore

    // push
    addi %r1, %r1, -64
    
    // save register to stack
    std %r4, 0(%r1) // old_vas_id
    std %r5, 8(%r1) // new_vas_id

    // call1

    mflr %r0
    std %r0, 32(%r1)

    ld %r3, 8(%r1) // new_vas_id
    li %r11, 7 // lv1_select_virtual_address_space

    sc 1

    std %r3, 16(%r1) // res1

    ld %r0, 32(%r1)
    mtlr %r0

    // call2

    mflr %r0
    std %r0, 32(%r1)

    ld %r3, 0(%r1) // old_vas_id
    li %r11, 7 // lv1_select_virtual_address_space

    sc 1

    std %r3, 24(%r1) // res2

    ld %r0, 32(%r1)
    mtlr %r0

    // set res1, res2 to output register
    ld %r4, 16(%r1) // res1
    ld %r5, 24(%r1) // res2

    // pop
    addi %r1, %r1, 64

    // enable interrupt
    li %r0, 0
    ori %r0, %r0, 0x8002
    eieio
    sync
    mtmsrd  %r0, 1

    blr

.global lv2_get_pir_do

lv2_get_pir_do:
    mfspr %r4, 286
    blr

.global lv2_glitcher_test_do

// input: r3 = addr, r4 = size
// output: r4 = writeCount

// r0 = value to write
// r5 = cur_addr
// r6 = writeCount
// r7 = temp
// r8 = end_addr

lv2_glitcher_test_do:
    
#if 1
    // disable interrupt
    li %r0, 2
    mtmsrd  %r0, 1
    eieio
    sync
#endif

    li %r0, 0x69

    // writeCount = 0
    li %r6, 0

    // return if size == 0
    li %r7, 0x00
    cmp 0, 0, %r4, %r7
    beq ret

    // cur_addr = addr
    mr %r5, %r3

    // end_addr = addr + size
    add %r8, %r3, %r4

loop:
    stb %r0, 0(%r5) // cur_addr[0] = r0

    // cur_addr += 1
    addi %r5, %r5, 1

    // writeCount += 1
    addi %r6, %r6, 1

    // return if cur_addr >= end_addr
    cmp 0, 0, %r5, %r8
    bge ret

    // loop
    b loop

ret:
    mr %r4, %r6 // copy writeCount to output

    eieio
    sync

#if 1
    // enable interrupt
    li %r0, 0
    ori %r0, %r0, 0x8002
    eieio
    sync
    mtmsrd  %r0, 1
#endif

    blr

// lv2_lv1_release_memory_intr_do(lpar_addr)

// input:
// r3 = lpar_addr

// output:
// r3 = res

.global lv2_lv1_release_memory_intr_do

lv2_lv1_release_memory_intr_do:

    // disable interrupt
    li %r0, 2
    mtmsrd  %r0, 1
    eieio
    sync

    // lv1_release_memory(lpar_addr)

    // push
    addi %r1, %r1, -64

    mflr %r0
    std %r0, 0(%r1)

    li %r11, 13 // lv1_release_memory
    sc 1

    eieio
    sync

    ld %r0, 0(%r1)
    mtlr %r0

    // pop
    addi %r1, %r1, 64

    // enable interrupt
    li %r0, 0
    ori %r0, %r0, 0x8002
    eieio
    sync
    mtmsrd  %r0, 1

    blr

// in: r3 = ea

.global lv2_dcbi_do

lv2_dcbi_do:
    eieio
    sync

    li %r0, 0
    dcbi %r0, %r3

    eieio
    sync

    blr