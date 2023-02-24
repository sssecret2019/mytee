/*
 * Copyright (c) 2013-2018, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <arch.h>
#include <asm_macros.S>
#include <context.h>
#include <cpu_data.h>
#include <ea_handle.h>
#include <interrupt_mgmt.h>
#include <platform_def.h>
#include <runtime_svc.h>
#include <smccc.h>

#define CONFIG_MYTEE 1	// Temp
#ifdef CONFIG_MYTEE
 #define EMUL_TTBR0      1
 #define EMUL_SCTLR      2
 #define EMUL_TTBR1      3
 #define EMUL_TCR        4
 #define EMUL_MAIR       5
 #define EMUL_VBAR       6
 #define EMUL_MEM_PTE	7
 #define EMUL_MEM_PMD	8
 #define EMUL_COPY_PMD	9
 #define EMUL_PMD_CLEAR	10
 #define EMUL_PMD_FREE 11
 #define EMUL_SET_PUD 12
 #define EMUL_PUD_CLEAR 13
 #define EMUL_NEW_PGD 14
 #define EMUL_PGD_FREE 15
 #define EMUL_RO_MEMSET 16
 #define EMUL_RO_MEMCPY 17
 
 #define MYTEE_RO_FAULT  21
 #define MYTEE_INIT 22
 #define MYTEE_KERNEL_TEXT_RO 98
 #define MYTEE_UNMMAP_FB 104
 #define MYTEE_REMMAP_FB 105
 #define MYTEE_GET_KERNEL_PHYS 106
 #define MYTEE_SHIELD_MMIO 109
 #define MYTEE_UNSHIELD_MMIO 110
 #define MYTEE_SHIELD_MMIO_4KB 111
 #define MYTEE_UNSHIELD_MMIO_4KB 112
 #define MYTEE_DMA_SET_NON_CACHEABLE 113
 #define MYTEE_SHIELD_MMIO_WITH_PHYS 114
 #define MYTEE_UNSHIELD_MMIO_WITH_PHYS 115
 #define MYTEE_COPY_CONTROL_BLOCKS_IN_USER 116
 
 #define MYTEE_SP_OFFSET_MMIO_PHYS_ADDR 0x0
 #define MYTEE_SP_OFFSET_SHIELD_SIZE 0x8
 #define MYTEE_SP_OFFSET_PAGE_INDEX 0x10
 #define MYTEE_SP_OFFSET_PAGE_INDEX_MUL_4KB 0x20
 #define MYTEE_SP_OFFSET_SHIELD_PAGE_NUM 0x20		// aslo be used at unshielding page
 #define MYTEE_SP_OFFSET_SHIELD_PAGE_NUM_4KB 0x28     // aslo be used at unshielding page
 #define MYTEE_SP_OFFSET_MMIO_DEVICE 0x30
 #define MYTEE_SP_OFFSET_UNSHIELD_SIZE 0x38
 #define MYTEE_SP_OFFSET_STATIC_TABLE_SAVED_ADDR 0x40
 
 #define MYTEE_MMIO_PHYS_ADDR_TPM_SPI 0x7E204000
 #define MYTEE_MMIO_PHYS_ADDR_FB 0x3e402000
 #define MYTEE_CONTROL_BLOCKS_LOG_PAGE 0x0fa00000
 
#define MYTEE_STAGE2_PAGE_TABLE_LEVEL2_2MB_DMA_PHYS 0x0F0F9000
#define MYTEE_STAGE2_PAGE_TABLE_LEVEL3_4KB_RPI3_DMA_CONTROLLER_PHYS_OFFSET 0x38
#define MYTEE_STAGE2_PAGE_TABLE_LEVEL2_2MB_USB_INTERNAL_DMA_PHYS 0x0F0FA000
#define MYTEE_STAGE2_PAGE_TABLE_LEVEL3_4KB_RPI3_INTERNAL_DMA_CONTROLLER_PHYS_OFFSET 0xC00
#define MYTEE_STAGE2_PAGE_TABLE_LEVEL1_1GB_PHYS 0x0F0F1000
#define MYTEE_STAGE2_PAGE_TABLE_LEVEL2_2MB_FRAME_BUFFER_1ST_PHYS_OFFSET 0xF90
#define MYTEE_STAGE2_PAGE_TABLE_LEVEL2_2MB_FRAME_BUFFER_2ND_PHYS_OFFSET 0xF98
#define MYTEE_STAGE2_PAGE_TABLE_LEVEL2_2MB_FRAME_BUFFER_3RD_PHYS_OFFSET 0xFA0
#define MYTEE_STAGE2_PAGE_TABLE_LEVEL2_2MB_FRAME_BUFFER_4TH_PHYS_OFFSET 0xFA8

#define MYTEE_STAGE2_PAGE_TABLE_LEVEL2_2MB_FRAME_BUFFER_1ST_2MB_PHYS 0x0F0F3000
#define MYTEE_STAGE2_PAGE_TABLE_LEVEL2_2MB_FRAME_BUFFER_LAST_2MB_PHYS 0x0F0F4000
#define MYTEE_STAGE2_PAGE_TABLE_LEVEL2_2MB_FAKE_FRAME_BUFFER_1ST_2MB_PHYS 0x0F0F5000
#define MYTEE_STAGE2_PAGE_TABLE_LEVEL2_2MB_FAKE_FRAME_BUFFER_LAST_2MB_PHYS 0x0F0F6000	// Over 0x0F0F6F58 is not map to framebuffer
 
 
#define OFFSET_2MB 0x200000
#define OFFSET_4KB 0x1000
#define MYTEE_RPI3_FRAME_BUFFER_ADDR_2ND_2MB_PHYS 0x3E600000
#define MYTEE_RPI3_FRAME_BUFFER_ADDR_3RD_2MB_PHYS 0x3E800000
#define MYTEE_FAKE_FRAME_BUFFER_2ND_PHYS 0xEA00000
#define MYTEE_FAKE_FRAME_BUFFER_3RD_PHYS 0xEC00000
#define STAGE2_TRANSLATION_LEVEL012_LOWER_ATTRIBUTE_ACCESS_PERMISSION_TO_RW 0x7FD
#define STAGE2_TRANSLATION_LEVEL012_LOWER_ATTRIBUTE_ACCESS_PERMISSION_TO_NA 0x73D
#define STAGE2_TRANSLATION_LEVEL3_LOWER_ATTRIBUTE_ACCESS_PERMISSION_TO_RW 0x7FF
#define STAGE2_TRANSLATION_LEVEL3_LOWER_ATTRIBUTE_ACCESS_PERMISSION_TO_NA 0x73F
#define MYTEE_HYP_DYNAMICALLY_ALLOCATED_STAGE2_PAGE_TABLE_4KB_PHYS 0x0F1F0000
#define MYTEE_HYP_DYNAMICALLY_ALLOCATED_STAGE2_PAGE_TABLE_4KB_INFORM_PHYS 0x0F0F0100

#define MYTEE_MMIO_CONTEXT_DEVICE_TPM 1			
#define MYTEE_MMIO_CONTEXT_DEVICE_FRAMBUFFER 2
 
#endif

	.globl	runtime_exceptions

	.globl	sync_exception_sp_el0
	.globl	irq_sp_el0
	.globl	fiq_sp_el0
	.globl	serror_sp_el0

	.globl	sync_exception_sp_elx
	.globl	irq_sp_elx
	.globl	fiq_sp_elx
	.globl	serror_sp_elx

	.globl	sync_exception_aarch64
	.globl	irq_aarch64
	.globl	fiq_aarch64
	.globl	serror_aarch64

	.globl	sync_exception_aarch32
	.globl	irq_aarch32
	.globl	fiq_aarch32
	.globl	serror_aarch32

	/*
	 * Macro that prepares entry to EL3 upon taking an exception.
	 *
	 * With RAS_EXTENSION, this macro synchronizes pending errors with an ESB
	 * instruction. When an error is thus synchronized, the handling is
	 * delegated to platform EA handler.
	 *
	 * Without RAS_EXTENSION, this macro just saves x30, and unmasks
	 * Asynchronous External Aborts.
	 */
	.macro check_mytee
	
	CMP	x0, 0x20	// 0x20 is SW
	BEQ	mytee_store	// mytee store reg and eret
	CMP	x0, 0x30	// 0x30 is NW
	BEQ	mytee_restore
	
	.endm
func mytee_store
	add	x1, sp, #CTX_MYTEE_OFFSET + CTX_MYTEE_LR
	str	x30, [x1]
	
	/* mytee store GP reg */
	add	x1, sp, #CTX_MYTEE_OFFSET + CTX_MYTEE_X4
	stp	x4, x5, [x1]
	
	add	x1, sp, #CTX_MYTEE_OFFSET + CTX_MYTEE_X6
	stp	x6, x7, [x1]
	
	add	x1, sp, #CTX_MYTEE_OFFSET + CTX_MYTEE_X8
	stp	x8, x9, [x1]
	
	add	x1, sp, #CTX_MYTEE_OFFSET + CTX_MYTEE_X10
	stp	x10, x11, [x1]
	
	add	x1, sp, #CTX_MYTEE_OFFSET + CTX_MYTEE_X12
	stp	x12, x13, [x1]
	
	add	x1, sp, #CTX_MYTEE_OFFSET + CTX_MYTEE_X14
	stp	x14, x15, [x1]
	
	add	x1, sp, #CTX_MYTEE_OFFSET + CTX_MYTEE_X16
	stp	x16, x17, [x1]
	
	add	x1, sp, #CTX_MYTEE_OFFSET + CTX_MYTEE_X18
	stp	x18, x19, [x1]
	
	add	x1, sp, #CTX_MYTEE_OFFSET + CTX_MYTEE_X20
	stp	x20, x21, [x1]
	
	add	x1, sp, #CTX_MYTEE_OFFSET + CTX_MYTEE_X22
	stp	x22, x23, [x1]
	
	add	x1, sp, #CTX_MYTEE_OFFSET + CTX_MYTEE_X24
	stp	x24, x25, [x1]
	
	add	x1, sp, #CTX_MYTEE_OFFSET + CTX_MYTEE_X26
	stp	x26, x27, [x1]
	
	add	x1, sp, #CTX_MYTEE_OFFSET + CTX_MYTEE_X28
	stp	x28, x29, [x1]
	
	mrs	x18, sp_el0
	add	x1, sp, #CTX_MYTEE_OFFSET + CTX_MYTEE_SP_EL0
	str	x18, [x1]
	
	b	el3_exit
endfunc mytee_store

func mytee_restore
	b	el3_exit
endfunc mytee_restore
	
	.macro check_and_unmask_ea
#if RAS_EXTENSION
	/* Synchronize pending External Aborts */
	esb

	/* Unmask the SError interrupt */
	msr	daifclr, #DAIF_ABT_BIT

	/*
	 * Explicitly save x30 so as to free up a register and to enable
	 * branching
	 */
	str	x30, [sp, #CTX_GPREGS_OFFSET + CTX_GPREG_LR]

	/* Check for SErrors synchronized by the ESB instruction */
	mrs	x30, DISR_EL1
	tbz	x30, #DISR_A_BIT, 1f

	/* Save GP registers and restore them afterwards */
	bl	save_gp_registers
	bl	handle_lower_el_ea_esb
	bl	restore_gp_registers

1:
#else
	/* Unmask the SError interrupt */
	msr	daifclr, #DAIF_ABT_BIT

	str	x30, [sp, #CTX_GPREGS_OFFSET + CTX_GPREG_LR]
#endif
	.endm

	/* ---------------------------------------------------------------------
	 * This macro handles Synchronous exceptions.
	 * Only SMC exceptions are supported.
	 * ---------------------------------------------------------------------
	 */
	.macro	handle_sync_exception
#if ENABLE_RUNTIME_INSTRUMENTATION
	/*
	 * Read the timestamp value and store it in per-cpu data. The value
	 * will be extracted from per-cpu data by the C level SMC handler and
	 * saved to the PMF timestamp region.
	 */
	mrs	x30, cntpct_el0
	str	x29, [sp, #CTX_GPREGS_OFFSET + CTX_GPREG_X29]
	mrs	x29, tpidr_el3
	str	x30, [x29, #CPU_DATA_PMF_TS0_OFFSET]
	ldr	x29, [sp, #CTX_GPREGS_OFFSET + CTX_GPREG_X29]
#endif

	mrs	x30, esr_el3
	ubfx	x30, x30, #ESR_EC_SHIFT, #ESR_EC_LENGTH

	/* Handle SMC exceptions separately from other synchronous exceptions */
	cmp	x30, #EC_AARCH32_SMC
	b.eq	smc_handler32

	cmp	x30, #EC_AARCH64_SMC
	b.eq	smc_handler64

	/* Synchronous exceptions other than the above are assumed to be EA */
	ldr	x30, [sp, #CTX_GPREGS_OFFSET + CTX_GPREG_LR]
	b	enter_lower_el_sync_ea
	.endm


	/* ---------------------------------------------------------------------
	 * This macro handles FIQ or IRQ interrupts i.e. EL3, S-EL1 and NS
	 * interrupts.
	 * ---------------------------------------------------------------------
	 */
	.macro	handle_interrupt_exception label
	bl	save_gp_registers
	/* Save the EL3 system registers needed to return from this exception */
	mrs	x0, spsr_el3
	mrs	x1, elr_el3
	stp	x0, x1, [sp, #CTX_EL3STATE_OFFSET + CTX_SPSR_EL3]

	/* Switch to the runtime stack i.e. SP_EL0 */
	ldr	x2, [sp, #CTX_EL3STATE_OFFSET + CTX_RUNTIME_SP]
	mov	x20, sp
	msr	spsel, #0
	mov	sp, x2

	/*
	 * Find out whether this is a valid interrupt type.
	 * If the interrupt controller reports a spurious interrupt then return
	 * to where we came from.
	 */
	bl	plat_ic_get_pending_interrupt_type
	cmp	x0, #INTR_TYPE_INVAL
	b.eq	interrupt_exit_\label

	/*
	 * Get the registered handler for this interrupt type.
	 * A NULL return value could be 'cause of the following conditions:
	 *
	 * a. An interrupt of a type was routed correctly but a handler for its
	 *    type was not registered.
	 *
	 * b. An interrupt of a type was not routed correctly so a handler for
	 *    its type was not registered.
	 *
	 * c. An interrupt of a type was routed correctly to EL3, but was
	 *    deasserted before its pending state could be read. Another
	 *    interrupt of a different type pended at the same time and its
	 *    type was reported as pending instead. However, a handler for this
	 *    type was not registered.
	 *
	 * a. and b. can only happen due to a programming error. The
	 * occurrence of c. could be beyond the control of Trusted Firmware.
	 * It makes sense to return from this exception instead of reporting an
	 * error.
	 */
	bl	get_interrupt_type_handler
	cbz	x0, interrupt_exit_\label
	mov	x21, x0

	mov	x0, #INTR_ID_UNAVAILABLE

	/* Set the current security state in the 'flags' parameter */
	mrs	x2, scr_el3
	ubfx	x1, x2, #0, #1

	/* Restore the reference to the 'handle' i.e. SP_EL3 */
	mov	x2, x20

	/* x3 will point to a cookie (not used now) */
	mov	x3, xzr

	/* Call the interrupt type handler */
	blr	x21

interrupt_exit_\label:
	/* Return from exception, possibly in a different security state */
	b	el3_exit

	.endm

#ifdef CONFIG_MYTEE
// Traverse the swapper page tables to find the entries for the input physical address
// GPRs x26-x29 are used

#define BLANK_PAGE_MAP_ADDR 0x0f130000
#define BLANK_PAGE_SPINLOCK_OFFSET 0x0
#define BLANK_PAGE_INDEX_OFFSET 0x4
#define BLANK_PAGE_PHY_ADDR_OFFSET 0x8
#define BLANK_PAGE_MYTEE_EN_FLAG_OFFSET 0x10
#define BLANK_PAGE_BITMAP_OFFSET 0x800
#define NUM_OF_PAGES	0x600
#define PGT_BITMAP_ADDR_OFFSET 0x1000
#define PGT_BITMAP_SPINLOCK_OFFSET 0x1008

/** 
 - Bitmap flag is 1 byte each 
 - Struct: |1st 4 bits: # of map to the page|2nd 4 bits: map types|
 - Types: 0: no map, 1: swapper pgd, 2: swapper pmd, 3: swapper pte, 
          4: [hyp, TEE, kernel text, and RO Bufs], 
          5: App pgd, 6: App pmd, 7: pte for app  
**/

#define MYTEE_NO_MAP 	0
#define MYTEE_SWP_PGD	1
#define MYTEE_SWP_PMD	2
#define MYTEE_SWP_PTE	3
#define MYTEE_RO_REGION	4
#define MYTEE_APP_PGD	5
#define MYTEE_APP_PMD	6
#define MYTEE_APP_PTE	7

#define SWAPPER_PGD_ADDR 0x3000
#define HYP_START_ADDR 0x0e800000
#define TEE_START_ADDR 0x100E0000
#define TEE_END_ADDR   0x11000000
//TODO: Get these info from mytee_init
#define KERNEL_TEXT_START_ADDR 0x00200000
#define KERNEL_TEXT_END_ADDR   0x00a00000


#define MYTEE_EXTRA_MEM_START    0xDA00000
#define MYTEE_EXTRA_MEM_SIZE     0x600000

#define   MYTEE_ROBUF_START      0xE000000
#define   MYTEE_ROBUF_SIZE       0x800000

#define MYTEE_RO_ALL_SIZE 	MYTEE_EXTRA_MEM_SIZE + MYTEE_ROBUF_SIZE

.macro set_mytee_init_flag
    mov w26, #BLANK_PAGE_MAP_ADDR	
    mov w27, #1
    str w27, [x26, #BLANK_PAGE_MYTEE_EN_FLAG_OFFSET]		    	 
.endm

.macro check_mytee_init_flag
    mov w26, #BLANK_PAGE_MAP_ADDR       
    ldr w26, [x26, #BLANK_PAGE_MYTEE_EN_FLAG_OFFSET]
.endm



.macro alloc_blank_page new_blank_page 
	mov w26, #BLANK_PAGE_MAP_ADDR
1:
	ldr w27, [x26, #BLANK_PAGE_SPINLOCK_OFFSET]	// Spinlock
	cmp w27, #1 
	beq 1b
	mov w27, #1
	str w27, [x26, #BLANK_PAGE_SPINLOCK_OFFSET]			// Lock
	dc cvac, x26

	ldr w27, [x26, #BLANK_PAGE_INDEX_OFFSET]	// Get the last index
	add w26, w26, #BLANK_PAGE_BITMAP_OFFSET

1:
	cmp w27, #NUM_OF_PAGES	//Circular buffer
	bne 2f
	mov w27, #0
2:
	ldrb w28, [x26, x27]	// GET BITMAP: 1byte				
	add w27, w27, #1
	cmp w28, #1
	beq 1b
	
	// x27: index
	mov w28, #BLANK_PAGE_MAP_ADDR
	add w26, w28, #BLANK_PAGE_BITMAP_OFFSET
	mov w29, #1
	sub w27, w27, #1
	strb w29, [x26, x27]	// Update the BITMAP			
	dc cvac, x26
	mov w26, w27
	lsl w26, w26, #12		//multiply 0x1000, page
	ldr w29, [x28, #BLANK_PAGE_PHY_ADDR_OFFSET]
	add w29, w29, w26	//get the blank page (phy)
	
	// return the result
	mov \new_blank_page, x29
	add w27, w27, #1
	str w27, [x28, #BLANK_PAGE_INDEX_OFFSET]

        str wzr, [x28, #BLANK_PAGE_SPINLOCK_OFFSET]                  // Lock release
	dsb sy
.endm

.macro free_blank_page ret_page
	mov w26, #BLANK_PAGE_MAP_ADDR
1:
	ldr w27, [x26, #BLANK_PAGE_SPINLOCK_OFFSET]     // Spinlock
        cmp w27, #1 
        beq 1b
        mov w27, #1
        str w27, [x26, #BLANK_PAGE_SPINLOCK_OFFSET]                     // Lock
        dc cvac, x26

	mov x26, \ret_page
	mov w27, #BLANK_PAGE_MAP_ADDR
	ldr w28, [x27, #BLANK_PAGE_PHY_ADDR_OFFSET]
	sub w26, w26, w28
	lsr w26, w26, #12	//page index
	add w27, w27, #BLANK_PAGE_BITMAP_OFFSET
	strb wzr, [x27, x26]
	
	mov w26, #512			// # of pte entries
1:
	cmp w26, #0			// clean up the free page
	beq 2f
	str xzr, [\ret_page], #8	
	sub w26, w26, #1
	b 1b
2:
	mov w28, #BLANK_PAGE_MAP_ADDR
        str wzr, [x28, #BLANK_PAGE_SPINLOCK_OFFSET]                  // Lock release
        dsb sy
.endm


// Find the pte of the input kernel address by traversing page tables based on TTBR1
// and set the pte to enforce the page for the input to RO.
// If the input (virtual) address is a linear address,
// it is fine to use this macro with the physical address.
 
.macro split_page_set_ro input_addr_phy, blank_page_phy, set_page, size
        and \input_addr_phy, \input_addr_phy, #0xffffffff
	lsr x26, \input_addr_phy, #30 
        mrs x27, ttbr1_el1
	and x27, x27, 0xfffff000
        lsl w26, w26, #3		// Multiply 8
	add w27, w27, w26		// Get the PGD entry address

        ldr x27, [x27]			// Get the PGD entry (X27: pmd base OR 1GB block)
        mov w28, #3	
        and w26, w27, w28		// Get the last two bits
	cmp w26, w28			// Check if the entry is a pmd base address        
        bne 50f
        
	// Here, the PGD entry points to the next table
	
  	lsr x26, \input_addr_phy, #21  // Get the pmd index
        ldr w28, =0x1ff
	and w26, w26, w28
        lsl w26, w26, #3
      
        ldr w28, =0xfffff000	       // Get the pmd entry address
        and w27, w27, w28
        add w27, w27, w26             
	mov w29, w27			// Save to split the block
        ldr x27, [x27]			// Get the pmd entry (x27: pte address or 2MB block)
        
	mov w28, #3			
        and w26, w27, w28
	cmp w26, w28
	bne 49f

	// The PMD entry is a pointer to the PTE.
	// Traverse the PTE to find the entry for the input_phy_addr
	// and set it to RO

	lsr x26, \input_addr_phy, #12   // Get the pte entry index
	ldr w28, =0x1ff
	and w26, w26, w28
	lsl w26, w26, #3
	
	ldr w28, =0xfffff000
	and w27, w27, w28
	add w27, w27, w26
	ldr x29, [x27]

	mov w28, #3
	and w26, w29, w28
	cmp w26, w28
	bne 50f		//if not #3, it's an invalid entry

	// Set the entry RO flag 
	ldr x26, =0xffffffffffffff3f
	and x29, x29, x26
	mov x26, #0x80
	orr x29, x29, x26
	str x29, [x27]
	dc cvac, x27
	mov x29, #1		// blank page is not used. 
	b 51f
	

49: // Check if the PMD entry is 2MB block. 
        mov w28, #1		
        cmp w26, w28
        bne 50f	     // Branch if it's not a 2MB block. Weird.
    
    // Check if the set_page flag is set
    // 1: Split the block to 4KB pages and set the corresponding page to RO
    // 0: set the 2MB blcok to RO 
	mov w28, #1   
        cmp \set_page, x28
        beq 47f			// split the block to pages

    // Here, the set_page flag == 0.
    // Check if the block is already write-protected.
    // x29: PMD addess, x27: PMD value
	tst w27, #0x80		// check if AP[1] is set (RO)
	beq 50f			// branch if it's not RO (Z flag == 1)
        mov x29, #1		// branch if it's already RO
        b 51f
50:
    // Else, check if the input is 2 MB aligned and size flag >= 2MB
	tst \input_addr_phy, #0x1fffff
	bne 47f				// branch if it's not aligned with 2 MB	
	cmp \size, #0x200000
	blt 47f				// branch if the size is less than 2 MB

    // Then, set the block to RO
	ldr x26, =0xffffffffffffff3f
        and x27, x27, x26
        mov x26, #0x80
        orr x27, x27, x26
        str x27, [x29]
        dc cvac, x29
        mov x29, #1             // blank page is not used. 
        b 51f
    
47:
   // Here, the pmd entry is for 2MB block, will be split
   // First, fill the blank page with pte entries for 4KB pages
        mov w26, #512	// # of pte entries
	orr x27, x27, 0x3	// set the attribute as a table
	mov x28, \blank_page_phy
1:	          
	cmp w26, #0
        beq 2f
	str x27, [x28], #8
	add x27, x27, #0x1000
	sub w26, w26, #1
	b 1b
2:
	orr x28, \blank_page_phy, #3	// set the pte flag
	str w28, [x29]	//Store lower 4 byte
	dc cvac, x29

	// Set the entry have RO attribute
	lsr x26, \input_addr_phy, #12   //Get the pte entry index
        ldr x28, =0x1ff
        and x26, x26, x28
        lsl x26, x26, #3

        ldr x28, =0xfffff000
	mov x27, \blank_page_phy
        and x27, x27, x28
        add x27, x27, x26
        ldr x29, [x27]

        ldr x26, =0xffffffffffffff3f
        and x29, x29, x26
        mov x26, #0x80
        orr x29, x29, x26
        str x29, [x27]   
        dc cvac, x27
        mov x29, #2             // blank page is used. 
        b 51f
	

50: // This is a 1GB block in the PGD. 
    // OR invalid entry in the PGD or PMD
    // We do not handle this case. Panic and die.
    mov x29, #0				//Set the error	flag
51: //Success

.endm
#endif

vector_base runtime_exceptions

	/* ---------------------------------------------------------------------
	 * Current EL with SP_EL0 : 0x0 - 0x200
	 * ---------------------------------------------------------------------
	 */
vector_entry sync_exception_sp_el0
	/* We don't expect any synchronous exceptions from EL3 */
	b	report_unhandled_exception
end_vector_entry sync_exception_sp_el0

vector_entry irq_sp_el0
	/*
	 * EL3 code is non-reentrant. Any asynchronous exception is a serious
	 * error. Loop infinitely.
	 */
	b	report_unhandled_interrupt
end_vector_entry irq_sp_el0


vector_entry fiq_sp_el0
	b	report_unhandled_interrupt
end_vector_entry fiq_sp_el0


vector_entry serror_sp_el0
	no_ret	plat_handle_el3_ea
end_vector_entry serror_sp_el0

	/* ---------------------------------------------------------------------
	 * Current EL with SP_ELx: 0x200 - 0x400
	 * ---------------------------------------------------------------------
	 */
vector_entry sync_exception_sp_elx
	/*
	 * This exception will trigger if anything went wrong during a previous
	 * exception entry or exit or while handling an earlier unexpected
	 * synchronous exception. There is a high probability that SP_EL3 is
	 * corrupted.
	 */
	b	report_unhandled_exception
end_vector_entry sync_exception_sp_elx

vector_entry irq_sp_elx
	b	report_unhandled_interrupt
end_vector_entry irq_sp_elx

vector_entry fiq_sp_elx
	b	report_unhandled_interrupt
end_vector_entry fiq_sp_elx

vector_entry serror_sp_elx
	no_ret	plat_handle_el3_ea
end_vector_entry serror_sp_elx

	/* ---------------------------------------------------------------------
	 * Lower EL using AArch64 : 0x400 - 0x600
	 * ---------------------------------------------------------------------
	 */
vector_entry sync_exception_aarch64
	/*
	 * This exception vector will be the entry point for SMCs and traps
	 * that are unhandled at lower ELs most commonly. SP_EL3 should point
	 * to a valid cpu context where the general purpose and system register
	 * state can be saved.
	 */
	check_and_unmask_ea
	handle_sync_exception
end_vector_entry sync_exception_aarch64

vector_entry irq_aarch64
	check_and_unmask_ea
	handle_interrupt_exception irq_aarch64
end_vector_entry irq_aarch64

vector_entry fiq_aarch64
	check_and_unmask_ea
	handle_interrupt_exception fiq_aarch64
end_vector_entry fiq_aarch64

vector_entry serror_aarch64
	msr	daifclr, #DAIF_ABT_BIT
	b	enter_lower_el_async_ea
end_vector_entry serror_aarch64

	/* ---------------------------------------------------------------------
	 * Lower EL using AArch32 : 0x600 - 0x800
	 * ---------------------------------------------------------------------
	 */
vector_entry sync_exception_aarch32
	/*
	 * This exception vector will be the entry point for SMCs and traps
	 * that are unhandled at lower ELs most commonly. SP_EL3 should point
	 * to a valid cpu context where the general purpose and system register
	 * state can be saved.
	 */
	check_mytee
	check_and_unmask_ea
	handle_sync_exception
end_vector_entry sync_exception_aarch32

vector_entry irq_aarch32
	check_and_unmask_ea
	handle_interrupt_exception irq_aarch32
end_vector_entry irq_aarch32

vector_entry fiq_aarch32
	check_and_unmask_ea
	handle_interrupt_exception fiq_aarch32
end_vector_entry fiq_aarch32

vector_entry serror_aarch32
	msr	daifclr, #DAIF_ABT_BIT
	b	enter_lower_el_async_ea
end_vector_entry serror_aarch32


	/* ---------------------------------------------------------------------
	 * This macro takes an argument in x16 that is the index in the
	 * 'rt_svc_descs_indices' array, checks that the value in the array is
	 * valid, and loads in x15 the pointer to the handler of that service.
	 * ---------------------------------------------------------------------
	 */
	.macro	load_rt_svc_desc_pointer
	/* Load descriptor index from array of indices */
	adr	x14, rt_svc_descs_indices
	ldrb	w15, [x14, x16]

#if SMCCC_MAJOR_VERSION == 1
	/* Any index greater than 127 is invalid. Check bit 7. */
	tbnz	w15, 7, smc_unknown
#elif SMCCC_MAJOR_VERSION == 2
	/* Verify that the top 3 bits of the loaded index are 0 (w15 <= 31) */
	cmp	w15, #31
	b.hi	smc_unknown
#endif /* SMCCC_MAJOR_VERSION */

	/*
	 * Get the descriptor using the index
	 * x11 = (base + off), w15 = index
	 *
	 * handler = (base + off) + (index << log2(size))
	 */
	adr	x11, (__RT_SVC_DESCS_START__ + RT_SVC_DESC_HANDLE)
	lsl	w10, w15, #RT_SVC_SIZE_LOG2
	ldr	x15, [x11, w10, uxtw]
	.endm

	/* ---------------------------------------------------------------------
	 * The following code handles secure monitor calls.
	 * Depending upon the execution state from where the SMC has been
	 * invoked, it frees some general purpose registers to perform the
	 * remaining tasks. They involve finding the runtime service handler
	 * that is the target of the SMC & switching to runtime stacks (SP_EL0)
	 * before calling the handler.
	 *
	 * Note that x30 has been explicitly saved and can be used here
	 * ---------------------------------------------------------------------
	 */
func smc_handler

smc_handler32:
#ifdef CONFIG_MYTEE
    /* Emaluation based on X0 value, which contains the type of privileged instructions     */
    /* 0: TTBR emuation, 1:    ...                                             */
//TODO: Validation before emulation.(tzrkp) 
	
    cmp     x0, EMUL_TTBR0
    bne    1f
    lsl	   x1, x1, #32
    orr    x1, x1, x2 
    msr     TTBR0_EL1, x1
    b	   6f
1:    
    cmp     x0, EMUL_SCTLR
    bne     2f
    msr     SCTLR_EL1, x1
    b 6f
2:
    cmp     x0, EMUL_TTBR1
    bne     3f
    msr     TTBR1_EL1, x1
   b 6f
3:
    cmp     x0, EMUL_TCR
    bne     4f
    msr     TCR_EL1, x1
  b 6f
4:
    cmp     x0, EMUL_MAIR
    bne     5f
    msr     MAIR_EL1, x1
  b 6f
5:
    cmp     x0, EMUL_VBAR
    bne     7f
    msr     VBAR_EL1, x1
  b 6f

7:
   // Emulation of setting a pte	
   // x1: dest, x2: val(low ). x3 val(high)
    cmp     x0, EMUL_MEM_PTE
    bne   8f	
    and x1, x1, 0xffffffff	
    at s1e1r, x1
    mrs x30, par_el1
    tst x30, #1
    bne 18f	
    and x30, x30, 0xfffff000
    and x1, x1, 0xfff
    add x30, x30, x1

// Verification

    mov x24, x30
    mov x25, #EMUL_MEM_PTE	 
    mov x0, x30
    bl verify_pg_dst 	
    cmp x29, #1
    beq 18f

    mov x24, x2
    mov x25, #EMUL_MEM_PTE       
    bl verify_pg_value
    cmp x29, #1				// Check error
    beq 18f	

   // Check x0 is mapped
    at s1e3w, x0
    mrs x30, par_el1	
    and x30, x30, #1
    cmp x30, #1			// par_el1.f bit 
    beq 18f

    str w2, [x0] 
    str w3, [x0, 4]
    dc cvac, x0
    dsb sy	

    b 6f //@
18:
    b 18b

8:	//Emulation of  PMD population
	// x1: dest, x2: val(low ). x3 val(high)

    cmp x0, EMUL_MEM_PMD
    bne 8f
// virt to phys
    and x1, x1, 0xffffffff
    at s1e1r, x1
    mrs x30, par_el1
    tst x30, #1
    bne	18f	
    and x30, x30, 0xfffff000
    and x1, x1, 0xfff
    add x30, x30, x1
    b 19f	
18:	//Fail to translate, we subtract page_offset
    and x30, x1, 0x7fffffff		
19:

// Verification
    mov x24, x30
    mov x25, #EMUL_MEM_PMD       
    mov x0, x30

    bl verify_pg_dst 
    cmp x29, #1
    beq 18f			// Check error

    mov x24, x2
    mov x25, #EMUL_MEM_PMD       

    bl verify_pg_value 
    cmp x29, #1		// Check error	
    beq 18f

    mov x30, x0	

//bitmap update

	and x0, x30, 0xfffff000
	mov x1, x30
	cmp x0, 0x6000		//1st swapper pmd entry
	bne 17f
        mov x24, x2
	mov x25, #MYTEE_SWP_PTE
	bl set_pgt_bitmap
	b 16f
17:
	cmp x0, 0x7000		//2nd swapper pmd entry
	bne 17f
 	mov x24, x2
	mov x25, #MYTEE_SWP_PTE
	bl set_pgt_bitmap
	b 16f
17:
	mov x24, x2
	mov x25, #MYTEE_APP_PTE
	bl set_pgt_bitmap 
16:

//TODO: set the pte RO

    mov x30, x1
    str w2, [x30]
    str w3, [x30, 4]
    dc cvac, x30
    dsb sy	
18:    
    b 6f

8:      //Emulation of  SET_PUD
        // x1: dest, x2: val(low ). x3 val(high)

    cmp x0, EMUL_SET_PUD
    bne 8f

// virt to phys
    and x1, x1, 0xffffffff
    at s1e1r, x1
    mrs x30, par_el1
    and x30, x30, 0xfffff000
    and x1, x1, 0xfff
    add x30, x30, x1
 
// Verification 
    mov x24, x30
    mov x25, #EMUL_SET_PUD       
    mov x0, x30
   
    bl verify_pg_dst 
    cmp x29, #1
    beq 18f         // Check error

    mov x24, x2
    mov x25, #EMUL_SET_PUD       

    bl verify_pg_value 
    cmp x29, #1     // Check error
    beq 18f

    mov x30, x0

// bitmap update
    mov x24, x2
    mov x25, #MYTEE_APP_PMD
    mov x0, x30		
    bl set_pgt_bitmap 

    str w2, [x0]
    str w3, [x0, 4]
    dc cvac, x0
    dsb sy	

18:    
    b 6f


8: // Emulation of pmd copy
   // x1: dest pmd address, x2: source pmd address

   cmp x0, EMUL_COPY_PMD
   bne 8f

// get target address
    and x1, x1, 0xffffffff
    at s1e1r, x1
    mrs x30, par_el1
    and x30, x30, 0xfffff000
    and x1, x1, 0xfff
    add x1, x30, x1

// Verify target
    mov x24, x1
    mov x25, #EMUL_COPY_PMD       	

    bl verify_pg_dst 
    cmp x29, #1
    beq 18f


// get source address
    and x2, x2, 0xffffffff
    at s1e1r, x2
    mrs x30, par_el1
    and x30, x30, 0xfffff000
    and x2, x2, 0xfff
    add x2, x30, x2

// Verify source: assume that it is swapper's pmd 
//TODO: check later 

/*
    and x0, x2, #0xfffff000
    cmp x0, #0x6000            //SWP pmd
    beq 17f
    cmp x0, #0x7000            //SWP pmd
    beq 17f
    b 18f
*/	
17:
    ldr x0, [x2]
    str x0, [x1]	
    dc cvac, x1
    dsb sy	

// bitmap update
    mov x24, x0
    mov x25, #MYTEE_SWP_PTE	
    bl set_pgt_bitmap	

18:    
    b 6f

// Emulation of pmd clear
// x1: pmd address
8: cmp x0, EMUL_PMD_CLEAR
   bne 8f

   and x1, x1, 0xffffffff
   at s1e1r, x1
   mrs x30, par_el1
   and x30, x30, 0xfffff000
   and x1, x1, 0xfff
   add x1, x30, x1

// Verification

   mov x24, x1
   mov x25, #EMUL_PMD_CLEAR       

   bl verify_pg_dst 
   cmp x29, #1
   beq 18f

// bitmap update if the entry is a table

   ldr w0, [x1]     			//alignment issue
   and x27, x0, #3
   cmp x27, #3
   bne 17f

  mov x24, x0
  bl clear_pgt_bitmap 

// emul clear
17:
   str xzr, [x1]
   dc  cvac, x1
   dsb sy	

18:
   b 6f


// Emulation of pmd free
// x1: pmd address
8: cmp x0, EMUL_PMD_FREE
   bne 8f

   and x1, x1, 0xffffffff
   at s1e1r, x1
   mrs x30, par_el1
   and x30, x30, 0xfffff000
   and x1, x1, 0xfff
   add x1, x30, x1

   //bitmap update: assume that pmd entries are cleared
   mov x24, x1
   bl clear_pgt_bitmap

   b 6f

// Emulation of pgd alloc. 
// x1: pgd address

// memset(new_pgd, 0, USER_PTRS_PER_PGD * sizeof(pgd_t));
// memcpy(new_pgd + USER_PTRS_PER_PGD, init_pgd + USER_PTRS_PER_PGD,
//                       (PTRS_PER_PGD - USER_PTRS_PER_PGD) * sizeof(pgd_t));
8:
   cmp x0, EMUL_NEW_PGD
   bne 8f
   
   and x1, x1, 0xffffffff
   at s1e1r, x1
   mrs x30, par_el1
   and x30, x30, 0xfffff000
   and x1, x1, 0xfff
   add x1, x30, x1

   // Verification

   mov x24, x1
   mov x25, #EMUL_NEW_PGD       

   bl verify_pg_dst 
   cmp x29, #1
   beq 18f

   // bitmap update
   mov x24, x1
   mov x25, #MYTEE_APP_PGD
   bl set_pgt_bitmap 

18:
   b 6f

// Emulation of pgd free. 
// X1: PGD address
8:
   cmp x0, EMUL_PGD_FREE
   bne 8f

   and x1, x1, 0xffffffff
   at s1e1r, x1
   mrs x30, par_el1
   and x30, x30, 0xfffff000
   and x1, x1, 0xfff
   add x1, x30, x1

   // Verification

   mov x24, x1
   mov x25, #EMUL_PGD_FREE       

   bl verify_pg_dst
   cmp x29, #1
   beq 18f

   // bitmap update
   mov x24, x1
   bl clear_pgt_bitmap

18:
   b 6f

// Emulation of pud clear
// x1: pud address
8: 
   cmp x0, EMUL_PUD_CLEAR
   bne 8f

   and x1, x1, 0xffffffff
   at s1e1r, x1
   mrs x30, par_el1
   and x30, x30, 0xfffff000
   and x1, x1, 0xfff
   add x1, x30, x1

//verification
   mov x24, x1
   mov x25, #EMUL_PUD_CLEAR       

   bl verify_pg_dst
   cmp x29, #1
   beq 18f

//bitmap update
   ldr x24, [x1]	//get the pmd address to be cleared
   bl clear_pgt_bitmap 

   str xzr, [x1]
   dc  cvac, x1
   dsb sy	

18:
   b 6f


// Emulation of memset for RO pages, which is 8 byte granularity
// Assume that it is invoked by the page table update (8 byte per each WR)
// TODO: check copy range
// X1: target, X2: value, X3: size
8:
   cmp x0, EMUL_RO_MEMSET
   bne 8f

   and x1, x1, 0xffffffff
   at s1e1r, x1
   mrs x30, par_el1
   and x30, x30, 0xfffff000
   and x1, x1, 0xfff
   add x1, x30, x1 

   mov x0, #8
22:
   cmp x3, x0
   ble 23f
   sub x3, x3, x0
   str x2, [x1], #8
   dc  cvac, x1
   dsb sy	
   b 22b

23:
   cmp x3, xzr
   beq 24f 	

   str x2, [x1]
   dc  cvac, x1
   dsb sy

24:
   b 6f

// Emulation of memcpy for RO pages, which is 8 BTYE granularity
// Assume that it is used by the page table update only
// X1: target, X2: source, X3: size
8:
   cmp x0, EMUL_RO_MEMCPY
   bne 8f

   and x1, x1, 0xffffffff
   at s1e1r, x1
   mrs x30, par_el1
   and x30, x30, 0xfffff000
   and x1, x1, 0xfff
   add x1, x30, x1

   and x2, x2, 0xffffffff
   at s1e1r, x2
   mrs x30, par_el1
   and x30, x30, 0xfffff000
   and x2, x2, 0xfff
   add x2, x30, x2

   mov x0, #8

22:
   cmp x3, x0
   ble 23f
   sub x3, x3, x0
   ldr x30, [x2], #8
   str x30, [x1], #8
   dc  cvac, x1
   dsb sy
   b 22b

23:
   cmp x3, xzr
   beq 24f

   ldr x30, [x2]
   str x30, [x1]
   dc  cvac, x1
   dsb sy

24:
   b 6f
.LTORG

// MYTEE_INIT invocation from init/main.c
// Protect swapper page tables and RO buffers
// Also, setup tht bitmap for allocated pages for page tables and protected objects
// x1: mytee_extra_mem_pgt_pa, x2: mytee_pgt_bitmap_addr

8: 
   cmp     x0, MYTEE_INIT
   bne     8f

    mov w3, #BLANK_PAGE_MAP_ADDR
    str w1, [x3, #BLANK_PAGE_PHY_ADDR_OFFSET]
    str w2, [x3, PGT_BITMAP_ADDR_OFFSET]	 		
    dsb sy
		

// TODO: Below logic can be put into a macro

// PGD for swapper
    alloc_blank_page x0	      // Assumes no alloc failure
    mov x1, #0x3000
    mov x24, #1			// set_page flag = 1
    mov x25, #0x1000		// size = 4KB
    split_page_set_ro x1, x0, x24, x25	
	
    cmp x29,#1			// Check if the free page is used
    bne 65f   
    free_blank_page x0	      // Free the unused blank page	
65:
    mov x25, #MYTEE_SWP_PGD
    mov x24, #0x3000
    bl set_pgt_bitmap 	

// 1st pmd for swapper    
    alloc_blank_page x0       // Assumes no alloc failure
    mov x1, #0x6000   
    mov x24, #1                 // set_page flag = 1
    mov x25, #0x1000            // size = 4KB
    split_page_set_ro x1, x0, x24, x25

    cmp x29,#1                  // Check if the free page is used
    bne 65f
    free_blank_page x0        // Free the unused blank page     
65:
    mov x25, #MYTEE_SWP_PMD
    mov x24, #0x6000
    bl set_pgt_bitmap

// 2nd pmd for swapper

    alloc_blank_page x0       // Assumes no alloc failure
    mov x1, #0x7000     
    mov x24, #1                 // set_page flag = 1
    mov x25, #0x1000            // size = 4KB
    split_page_set_ro x1, x0, x24, x25

    cmp x29,#1                  // Check if the free page is used
    bne 65f
    free_blank_page x0        // Free the unused blank page     
65:
    mov x25, #MYTEE_SWP_PMD
    mov x24, #0x7000
    bl set_pgt_bitmap

// TODO: pte for swapper

// set ROBUF region to readonly
    /// get robuf address
    mov x1, #MYTEE_EXTRA_MEM_START
    mov x2, #MYTEE_RO_ALL_SIZE	
 
77:
    cmp x2, xzr
    beq 65f                     // Loop out

    alloc_blank_page x0       // Assumes no alloc failure
    mov x24, xzr                 // set_page flag = 0
    mov x25, x2            // Remaining size for setting RO
    split_page_set_ro x1, x0, x24, x25

    cmp x29,#1                  // Check if the free page is used
    bne 64f
    free_blank_page x0        // Free the unused blank page     
64:
    mov x25, #MYTEE_RO_REGION
    mov x24, x1
    bl set_pgt_bitmap
    	
    add x1, x1, #0x1000
    sub x2, x2, #0x1000
    b 77b
    		
65:
    set_mytee_init_flag	
 
    tlbi alle1 

    ldr     x26, [sp, #CTX_GPREGS_OFFSET + CTX_GPREG_X26]
    ldr     x27, [sp, #CTX_GPREGS_OFFSET + CTX_GPREG_X27]
    ldr     x28, [sp, #CTX_GPREGS_OFFSET + CTX_GPREG_X28]
    ldr     x29, [sp, #CTX_GPREGS_OFFSET + CTX_GPREG_X29]
    ldr     x30, [sp, #CTX_GPREGS_OFFSET + CTX_GPREG_LR]
    eret 

.LTORG

// Kernel text RO using S2-paging is not mandatory
// because the RKP is adopted
8:
    cmp     x0, MYTEE_KERNEL_TEXT_RO
    bne     9f
    
    sub w0, w2, w1
    lsr w0, w0, 21
    mov w0, 10             // Experimentally 10 pages are ROed
    
    ldr w1, =0x0f0f2000 		//3rd table 
    ldr w2, =0xffffff7f
    
78:     
	cmp w0, 0
        beq 79f
        ldr w3, [x1]                    //get lower bits of entry
        
	and w3, w3, w2
        str w3, [x1]                    //store masked entry
    	dc cvac, x1
        add w1, w1, 8      
        sub w0, w0, 1
        b 78b

// DMA TEST(BCM): external engine
79:
        ldr w1, =MYTEE_STAGE2_PAGE_TABLE_LEVEL2_2MB_DMA_PHYS
        ldr w3, =MYTEE_STAGE2_PAGE_TABLE_LEVEL3_4KB_RPI3_DMA_CONTROLLER_PHYS_OFFSET
        orr w1, w1, w3			//ldr w1, =0x0f0f9038		//3rd table
        ldr w2, =0xffffff7f		// RO
        ldr  w3, [x1]
        and  w3, w3, w2
        str  w3, [x1]
    	dc cvac, x1

// DMA TEST(DCW-OTG): USB DMA engine
        ldr w1, =MYTEE_STAGE2_PAGE_TABLE_LEVEL2_2MB_USB_INTERNAL_DMA_PHYS
        ldr w3, =MYTEE_STAGE2_PAGE_TABLE_LEVEL3_4KB_RPI3_INTERNAL_DMA_CONTROLLER_PHYS_OFFSET
        orr w1, w1, w3	 //ldr w1, =0x0f0fac00			//3rd table
        ldr w2, =0xffffff7f			// RO
        mov w0, 8
78:     
	cmp w0, 0
        beq 79f
   
        ldr  w3, [x1]
        and  w3, w3, w2
        str  w3, [x1]
    	dc cvac, x1

        sub w0, w0, 1
        add w1, w1, 8
        b 78b
79:     tlbi alle1

6:
    ldr     x26, [sp, #CTX_GPREGS_OFFSET + CTX_GPREG_X26]
    ldr     x27, [sp, #CTX_GPREGS_OFFSET + CTX_GPREG_X27]
    ldr     x28, [sp, #CTX_GPREGS_OFFSET + CTX_GPREG_X28]
    ldr     x29, [sp, #CTX_GPREGS_OFFSET + CTX_GPREG_X29]
    ldr     x30, [sp, #CTX_GPREGS_OFFSET + CTX_GPREG_LR]
    eret

9:	cmp	x0, #MYTEE_UNMMAP_FB
	bne	10f
	
	ldr	w0, =MYTEE_STAGE2_PAGE_TABLE_LEVEL1_1GB_PHYS
	ldr	w2, =MYTEE_STAGE2_PAGE_TABLE_LEVEL2_2MB_FRAME_BUFFER_1ST_PHYS_OFFSET
	orr	w0, w0, w2
	ldr	w1, =MYTEE_STAGE2_PAGE_TABLE_LEVEL2_2MB_FAKE_FRAME_BUFFER_1ST_2MB_PHYS
	ldr	w2, =0x3
	orr	w1, w1, w2
	str	w1, [x0]
	
	ldr	w0, =MYTEE_STAGE2_PAGE_TABLE_LEVEL1_1GB_PHYS
	ldr	w2, =MYTEE_STAGE2_PAGE_TABLE_LEVEL2_2MB_FRAME_BUFFER_2ND_PHYS_OFFSET
	orr	w0, w0, w2
	ldr	w1, =MYTEE_FAKE_FRAME_BUFFER_2ND_PHYS
	ldr	w2, =STAGE2_TRANSLATION_LEVEL012_LOWER_ATTRIBUTE_ACCESS_PERMISSION_TO_RW
	orr	w1, w1, w2
	str	w1, [x0]

	ldr	w0, =MYTEE_STAGE2_PAGE_TABLE_LEVEL1_1GB_PHYS
	ldr	w2, =MYTEE_STAGE2_PAGE_TABLE_LEVEL2_2MB_FRAME_BUFFER_3RD_PHYS_OFFSET
	orr	w0, w0, w2
	ldr	w1, =MYTEE_FAKE_FRAME_BUFFER_3RD_PHYS
	ldr	w2, =STAGE2_TRANSLATION_LEVEL012_LOWER_ATTRIBUTE_ACCESS_PERMISSION_TO_RW
	orr	w1, w1, w2
	str	w1, [x0]
	
	ldr	w0, =MYTEE_STAGE2_PAGE_TABLE_LEVEL1_1GB_PHYS
	ldr	w2, =MYTEE_STAGE2_PAGE_TABLE_LEVEL2_2MB_FRAME_BUFFER_4TH_PHYS_OFFSET
	orr	w0, w0, w2
	ldr	w1, =MYTEE_STAGE2_PAGE_TABLE_LEVEL2_2MB_FAKE_FRAME_BUFFER_LAST_2MB_PHYS
	ldr	w2, =0x3
	orr	w1, w1, w2
	str	w1, [x0]
	
	ldr	w0, =MYTEE_STAGE2_PAGE_TABLE_LEVEL1_1GB_PHYS
	ldr	w2, =MYTEE_STAGE2_PAGE_TABLE_LEVEL2_2MB_FRAME_BUFFER_1ST_PHYS_OFFSET
	orr	w0, w0, w2
	dc 	cvac, x0		 // Clean single data cache line
	
	eret
.LTORG 

10:	cmp	x0, #MYTEE_REMMAP_FB
	bne	14f
	
	ldr	w0, =MYTEE_STAGE2_PAGE_TABLE_LEVEL1_1GB_PHYS
	ldr	w2, =MYTEE_STAGE2_PAGE_TABLE_LEVEL2_2MB_FRAME_BUFFER_1ST_PHYS_OFFSET
	orr	w0, w0, w2
	ldr	w1, =MYTEE_STAGE2_PAGE_TABLE_LEVEL2_2MB_FRAME_BUFFER_1ST_2MB_PHYS
	ldr	w2, =0x3
	orr	w1, w1, w2
	str	w1, [x0]
	
	ldr	w0, =MYTEE_STAGE2_PAGE_TABLE_LEVEL1_1GB_PHYS
	ldr	w2, =MYTEE_STAGE2_PAGE_TABLE_LEVEL2_2MB_FRAME_BUFFER_2ND_PHYS_OFFSET
	orr	w0, w0, w2
	ldr	w1, =MYTEE_RPI3_FRAME_BUFFER_ADDR_2ND_2MB_PHYS
	ldr	w2, =STAGE2_TRANSLATION_LEVEL012_LOWER_ATTRIBUTE_ACCESS_PERMISSION_TO_RW
	orr	w1, w1, w2	
	str	w1, [x0]

	ldr	w0, =MYTEE_STAGE2_PAGE_TABLE_LEVEL1_1GB_PHYS
	ldr	w2, =MYTEE_STAGE2_PAGE_TABLE_LEVEL2_2MB_FRAME_BUFFER_3RD_PHYS_OFFSET
	orr	w0, w0, w2
	ldr	w1, =MYTEE_RPI3_FRAME_BUFFER_ADDR_3RD_2MB_PHYS
	ldr	w2, =STAGE2_TRANSLATION_LEVEL012_LOWER_ATTRIBUTE_ACCESS_PERMISSION_TO_RW
	orr	w1, w1, w2
	str	w1, [x0]
	
	ldr	w0, =MYTEE_STAGE2_PAGE_TABLE_LEVEL1_1GB_PHYS
	ldr	w2, =MYTEE_STAGE2_PAGE_TABLE_LEVEL2_2MB_FRAME_BUFFER_4TH_PHYS_OFFSET
	orr	w0, w0, w2
	ldr	w1, =MYTEE_STAGE2_PAGE_TABLE_LEVEL2_2MB_FRAME_BUFFER_LAST_2MB_PHYS
	ldr	w2, =0x3
	orr	w1, w1, w2
	str	w1, [x0]
	
	ldr	w0, =MYTEE_STAGE2_PAGE_TABLE_LEVEL1_1GB_PHYS
	ldr	w2, =MYTEE_STAGE2_PAGE_TABLE_LEVEL2_2MB_FRAME_BUFFER_1ST_PHYS_OFFSET
	orr	w0, w0, w2		
	dc 	cvac, x0		 // Clean single data cache line
	
	eret
.LTORG	

14:	cmp	w0, #MYTEE_SHIELD_MMIO
	bne	15f
	
	sub	sp, sp, #(8 * 10)

	at	s1e1r, x1
    	mrs	x30, par_el1
    	mov	x0, x30
    	and	x0, x0, #0x1
    	cmp	x0, #0x1
    	beq	exit		// Address translation aborted
    	and	x30, x30, 0xfffffffff000
	and	x1, x1, 0xfff
    	add	x1, x1, x30
    	
	str	x1, [sp, #MYTEE_SP_OFFSET_MMIO_PHYS_ADDR]	// sp = mmio addr(phys), sp+0x8 = size, sp+0x10 = page index
	str	x3, [sp, #MYTEE_SP_OFFSET_SHIELD_SIZE]
	
	mov	w0, #0					// Base addr : 3e402000, shield size : 0x400000 
							
calc_page_num:						
	add	w0, w0, #1
	subs	w3, w3, OFFSET_2MB
	cmp	w3, #0x0
	bgt	calc_page_num
	
	mov	w3, w0
	str	x3, [sp, #MYTEE_SP_OFFSET_SHIELD_PAGE_NUM]
	// Change mmio momory table entry (2MB) with no access permission
	ldr	w0, =0x3FE00000	// Maksing value
	and	w0, w0, w1	
	mov	w3, w0
	lsr	w0, w0, 0x14		
	ldr	w1, =0x4
	mul	w0, w0, w1		
	
	ldr	w1, =MYTEE_STAGE2_PAGE_TABLE_LEVEL1_1GB_PHYS	// Find index of mmio memory in 2 MB table
	add	w0, w0, w1		// r0 = 0x0f0f1d80 = 0x360007fd => RW
	
	// Check if w0 is table entry (Flag bits are 0b11)
	// Then, w0 is already mapped to 4kb page, goto exit and TODO: report an error
	ldr	w1, [x0]
	and	w1, w1, 0x00000003	
	cmp	w1, #0x3
	beq	exit
		
	mov	w2, #0		// Permission bits: 0x7fd is for R/W and x73d indicates no-access
	ldr	w2, =STAGE2_TRANSLATION_LEVEL012_LOWER_ATTRIBUTE_ACCESS_PERMISSION_TO_NA

	ldr	x1, [sp, #MYTEE_SP_OFFSET_SHIELD_PAGE_NUM]	
	orr	w3, w3, w2		// r3 = 0x3600073d
	ldr	w2, =OFFSET_2MB
	
shield:
	str	w3, [x0]		 // r0: 0x0f0f1d80 => r3: 0x3600073d
	dc 	cvac, x0		 // Clean single data cache line
	add	w3, w3, w2
	add	w0, w0, #0x8
	subs	w1, w1, #1

	bne	shield

	tlbi alle1
exit:
	add	sp, sp, #(8 * 10)
	ldr     x30, [sp, #CTX_GPREGS_OFFSET + CTX_GPREG_LR]
	eret
.LTORG

15:	cmp	w0, #MYTEE_UNSHIELD_MMIO
	bne	16f

	sub	sp, sp, #(8 * 10)

	at	s1e1r, x1
    	mrs	x30, par_el1
    	mov	x0, x30
    	and	x0, x0, #0x1
    	cmp	x0, #0x1
    	beq	exit2		// Address translation aborted
    	and	x30, x30, 0xfffffffff000
	and	x1, x1, 0xfff
    	add	x1, x1, x30
    	
	str	x1, [sp, #MYTEE_SP_OFFSET_MMIO_PHYS_ADDR]			// sp = mmio addr(phys), sp+0x8 = size, sp+0x10 = page index
	str	x3, [sp, #MYTEE_SP_OFFSET_SHIELD_SIZE]
	
	mov	w0, #0					// base addr : 3e402000, shield size : 0x400000 							
calc_page_num2:						
	add	w0, w0, #1
	subs	w3, w3, OFFSET_2MB
	cmp	w3, #0x0
	bgt	calc_page_num2
	
	mov	w3, w0
	str	x3, [sp, #MYTEE_SP_OFFSET_SHIELD_PAGE_NUM]
	// Change mmio momory table entry (2MB) with no access permission
	ldr	w0, =0x3FE00000 // Maksing value
	and	w0, w0, w1	
	mov	w3, w0
	lsr	w0, w0, 0x14		
	ldr	w1, =0x4
	mul	w0, w0, w1		
	
	ldr	w1, =MYTEE_STAGE2_PAGE_TABLE_LEVEL1_1GB_PHYS 	// Find index of mmio memory in 2 MB table
	add	w0, w0, w1		
	
	// Check if w0 is table entry (Flag is 0b11)
	// Then, w0 is already mapped 4kb, goto exit and TODO: report an error
	ldr	w1, [x0]
	and	w1, w1, 0x00000003	
	cmp	w1, #0x3
	beq	exit2
		
	mov	w2, #0		// 0x7fd: R/W, 0x73d: no-access
	ldr	w2, =STAGE2_TRANSLATION_LEVEL012_LOWER_ATTRIBUTE_ACCESS_PERMISSION_TO_RW

	ldr	x1, [sp, #MYTEE_SP_OFFSET_SHIELD_PAGE_NUM]	
	orr	w3, w3, w2		// r3 = 0x3600073d
	ldr	w2, =OFFSET_2MB
	
unshield:
	str	w3, [x0]		// 0x0f0f1d80 => 0x3600073d
	dc 	cvac, x0		 // Clean single data cache line
	add	w3, w3, w2
	add	w0, w0, #0x8
	subs	w1, w1, #1	

	bne	unshield
	
	tlbi alle1
exit2:
	add	sp, sp, #(8 * 10)
	ldr     x30, [sp, #CTX_GPREGS_OFFSET + CTX_GPREG_LR]
	eret
.LTORG

16:	cmp	w0, #MYTEE_SHIELD_MMIO_4KB
	bne	17f

	sub	sp, sp, #(8 * 10)

//	Lookup available page tables between 0x0f1f0000 and 0x0f1fa000.  
//      0x0f0f0100 stores the bitmap for the allocation
//	If the bitmap value is 0x0, the corresponding page is available
//	Else, the corresponding page is used by some devices depending on the bitmap value
//	[Bitmap value ==> 0x1: TPM, 0x2: framebuffer...]

	at	s1e1r, x1
    	mrs	x30, par_el1
    	mov	x0, x30
    	and	x0, x0, #0x1
    	cmp	x0, #0x1
    	beq	exit3		// Address translation aborted
    	and	x30, x30, 0xfffffffff000
	and	x1, x1, 0xfff
    	add	x1, x1, x30
    	    	
	str	x1, [sp, #MYTEE_SP_OFFSET_MMIO_PHYS_ADDR]			// sp = mmio addr(phys), sp+0x8 = size, sp+0x10 = page index
	str	x2, [sp, #MYTEE_SP_OFFSET_MMIO_DEVICE]
	str	x3, [sp, #MYTEE_SP_OFFSET_SHIELD_SIZE]
		
	ldr	w0, =MYTEE_HYP_DYNAMICALLY_ALLOCATED_STAGE2_PAGE_TABLE_4KB_INFORM_PHYS
	ldr	w3, =0x0		
  
check_double_mapping:
	ldr	w1, [x0]		
	cmp	x1, x2
	beq	exit3			// x0 page is already mapped to  4KB page
	add	w0, w0, #0x8
	add	w3, w3, #0x1
	cmp	w3, #0xa			// r0 = page inform addr, r1 = imformaion, r2 = mmio_device, r3= page index
	bne	check_double_mapping
	
	ldr	x1, [sp, #MYTEE_SP_OFFSET_MMIO_PHYS_ADDR]
	ldr	x3, [sp, #MYTEE_SP_OFFSET_SHIELD_SIZE]
	
	mov	w0, #0					
							
calc_page_num3:						
	add	w0, w0, #1
	subs	w3, w3, OFFSET_4KB
	cmp	w3, #0x0
	bgt	calc_page_num3
	
	mov	w3, w0
	str	x3, [sp, #MYTEE_SP_OFFSET_SHIELD_PAGE_NUM_4KB]

	// str	x3, [sp, #0x8]
	ldr	w0, =MYTEE_HYP_DYNAMICALLY_ALLOCATED_STAGE2_PAGE_TABLE_4KB_INFORM_PHYS
	ldr	w3, =0x0		
  
loop:
	ldr	w1, [x0]		
	cmp	x1, #0x0
	beq	notused
	add	w0, w0, #0x8
	add	w3, w3, #0x1
	cmp	w3, #0x9			// r0 = page inform addr, r1 = imformaion, r2 = mmio_device, r3= page index
	bne	loop
	ldr	x0, =0xffffffffffffffff		// Fail to allocate memory, no page available
	b	exit3
	
notused:
	ldr	x2, [sp, #MYTEE_SP_OFFSET_MMIO_DEVICE]
	str	w2, [x0]				// Save mmio_device flag
	dc 	cvac, x0		                // Clean single data cache line
	ldr	x1, [sp, #MYTEE_SP_OFFSET_MMIO_PHYS_ADDR]
	
	// r3 = page index
	// r1 = mmio addr
	
	ldr	w0, =0x3FE00000
	and	w0, w0, w1	
	add	w0, w0, #STAGE2_TRANSLATION_LEVEL3_LOWER_ATTRIBUTE_ACCESS_PERMISSION_TO_RW
	
	mov	w2, #0
	orr	w2, w2, 512	// 4KB * 512 = 2MB

	ldr	w1, =OFFSET_4KB
	mul	w3, w3, w1
	mov	w1, #0
	ldr	w1, =MYTEE_HYP_DYNAMICALLY_ALLOCATED_STAGE2_PAGE_TABLE_4KB_PHYS // r0 = 0x360007ff, r1=0x0f1f0000, r2=512, r3 = page index
	add	w1, w1, w3	// r0 = 0x360007ff, r1= page addr, r2= 512, r3 = page index
	str	x3, [sp, #MYTEE_SP_OFFSET_PAGE_INDEX_MUL_4KB]		// Save page_index 
	mov	w3, #0
	sub	w1, w1, #0x8
loop2:	
	add	w1, w1, #0x8	 
	str	w0, [x1]
	str	w3, [x1, #0x4]
	dc 	cvac, x1
	
	add	w0, w0, #OFFSET_4KB	 
	subs 	w2, w2, #1
	bne	loop2
	
	ldr	x1, [sp, #MYTEE_SP_OFFSET_MMIO_PHYS_ADDR]
	
	// change 2mb table to 4kb table
	ldr	w0, =0x3FE00000
	and	w0, w0, w1	
	mov	w2, w1		
	lsr	w0, w0, 0x14		
	ldr	w1, =0x4
	mul	w0, w0, w1		
	
	ldr	w1, =MYTEE_STAGE2_PAGE_TABLE_LEVEL1_1GB_PHYS	// Find index of mmio memory in 2 MB table
	add	w0, w0, w1		
	
	ldr	w1, [x0]	
	and	w1, w1, 0x00000003	// Check if it's already mapped to 4kb page (at boot time)
	cmp	w1, #0x3
	bne	start_dynamic_4kb_shield

	// Save the mapped 4 KB table address 
	ldr	w1, =MYTEE_HYP_DYNAMICALLY_ALLOCATED_STAGE2_PAGE_TABLE_4KB_INFORM_PHYS
	add	w1, w1, #0x4	
	ldr	x3, [sp, #MYTEE_SP_OFFSET_PAGE_INDEX_MUL_4KB]
	lsr	w3, w3, #0x4
	add	w1, w1, w3
	ldr	w0, [x0]
	and	w0, w0, #0xFFFFFFFC
	str	w0, [x1]
	dc 	cvac, x1
	
	// Change the permission of MMIO memory table (4KB) to no access
	ldr	w1, =0x1FF000		// To find the 4 KB page index
	and	w2, w2, w1		
	lsr	w2, w2, 0xc		
	ldr	w1, =0x8
	mul	w2, w2, w1		
	add	w0, w0, w2
	
	ldr	w3, [x0]			// r3 = 0x3f00b7ff	
	ldr	w1, =STAGE2_TRANSLATION_LEVEL3_LOWER_ATTRIBUTE_ACCESS_PERMISSION_TO_NA			
	and	w3, w3, #0xFFFFF000		// r3 = 0x3f00b000
	orr	w3, w3, w1			// r3 = 0x3f00b73f
	ldr	x1, =OFFSET_4KB
	ldr	x2, [sp, #MYTEE_SP_OFFSET_SHIELD_PAGE_NUM_4KB]
	
static_table_4kb_shield:
	str	w3, [x0]		 // x0:0x0f0f9058, x3:0x3f00b73f
	dc 	cvac, x0		 // Clean single data cache line
	add	w3, w3, w1
	add	w0, w0, #0x8
	subs	w2, w2, #1
	bne	static_table_4kb_shield
	tlbi	alle1
	b	exit3
	
start_dynamic_4kb_shield:		// r0 = 0x0f0f1d80, r2 = 0x36123456, r3 = page index
					
	ldr	w1, =0x1FF000	// Find the 4 KB index
	and	w2, w2, w1	
	lsr	w2, w2, 0xc		
	
	ldr	w1, =MYTEE_HYP_DYNAMICALLY_ALLOCATED_STAGE2_PAGE_TABLE_4KB_PHYS
	add	w1, w1, #0x3
	ldr	x3, [sp, #MYTEE_SP_OFFSET_PAGE_INDEX_MUL_4KB]
	add	w1, w1, w3
					
	str	w1, [x0]		
	dc 	cvac, x0		 // Clean single data cache line
	
	// change mmio momory table(4KB) to permission with No Access
	ldr	w3, =0x8
	mul	w2, w2, w3		
	
	ldr	w3, [x0]			// r3 = 0x0f0f7003
	and	w3, w3, 0xfffffffc		// last 2 bits to 0x00 -> r3 = 0x0f1f0000
	add	w2, w2, w3		
	
	mov	w1, #0		
	ldr	w1, =STAGE2_TRANSLATION_LEVEL3_LOWER_ATTRIBUTE_ACCESS_PERMISSION_TO_NA	
	
	ldr	x3, [sp, #MYTEE_SP_OFFSET_MMIO_PHYS_ADDR]
		
	and	w3, w3, #0xFFFFF000		
	orr	w3, w3, w1		
	
	ldr	x0, [sp, #MYTEE_SP_OFFSET_SHIELD_PAGE_NUM_4KB]
	ldr	x1, =OFFSET_4KB

shield_4kb:
	str	w3, [x2]		 
	dc 	cvac, x2		 // Clean single data cache line
	add	w3, w3, w1
	add	w2, w2, #0x8
	subs	w0, w0, #1
	bne	shield_4kb
	tlbi alle1
exit3:
	add	sp, sp, #(8 * 10)
	ldr     x30, [sp, #CTX_GPREGS_OFFSET + CTX_GPREG_LR]
	eret
.LTORG

17:	cmp	w0, #MYTEE_UNSHIELD_MMIO_4KB
	bne	18f
	
	sub	sp, sp, #(8 * 10)
	
	at	s1e1r, x1
    	mrs	x30, par_el1
    	mov	x0, x30
    	and	x0, x0, #0x1
    	cmp	x0, #0x1
    	beq	exit4		// Address translation aborted
    	and	x30, x30, 0xfffffffff000
	and	x1, x1, 0xfff
    	add	x1, x1, x30

	str	x1, [sp, #MYTEE_SP_OFFSET_MMIO_PHYS_ADDR]	// sp = mmio addr(phys), sp+0x8 = size, sp+0x10 = page index
	str	x2, [sp, #MYTEE_SP_OFFSET_MMIO_DEVICE]
	str	x3, [sp, #MYTEE_SP_OFFSET_UNSHIELD_SIZE]

	add	x2, x2, #0x10			// Verify the flag written by TA
	ldr	w0, =MYTEE_HYP_DYNAMICALLY_ALLOCATED_STAGE2_PAGE_TABLE_4KB_INFORM_PHYS	
	ldr	w3, =0x0	
	
check_shielded_page:
	ldr	w1, [x0]		
	cmp	x1, x2		// Find shielded memory for MMIO_DEVICE and check the TA verify flag
	beq	used
	add	w0, w0, #0x8
	add	w3, w3, #0x1
	cmp	w3, #0x10	// r0 = page inform addr, r1 = informaion, r2 = mmio_device, r3= page index
	bne	check_shielded_page
	ldr	x0, =0xffffffffffffffff	// Fail to find shielded memory or MMIO result is not consumed by TA (flag is not set).
	b	exit4
	
used:
	add	w0, w0, #0x4
	ldr	w1, [x0]
	cmp	w1, #0x0
	beq	non_static_page_shield
	str	x0, [sp, #MYTEE_SP_OFFSET_STATIC_TABLE_SAVED_ADDR]
	
	// Caculate page size to shield
	ldr	x3, [sp, #MYTEE_SP_OFFSET_UNSHIELD_SIZE]	// x0 = 0f0f0100, x1 = 0f0f9000	
	mov	w2, #0
calc_page_num6:						
	add	w2, w2, #1
	subs	w3, w3, OFFSET_4KB
	cmp	w3, #0x0
	bgt	calc_page_num6
	str	x2, [sp, #MYTEE_SP_OFFSET_PAGE_INDEX]
	
	// change mmio momory table(4KB) to permission with R/W
	ldr	x2, [sp, #MYTEE_SP_OFFSET_MMIO_PHYS_ADDR]
	ldr	w3, =0x1FF000		
	and	w2, w2, w3		
	lsr	w2, w2, 0xc		
	ldr	w3, =0x8
	mul	w2, w2, w3		
	add	w1, w1, w2		
	
	ldr	w3, [x1]
	ldr	w0, =STAGE2_TRANSLATION_LEVEL3_LOWER_ATTRIBUTE_ACCESS_PERMISSION_TO_RW		
	and	w3, w3, #0xFFFFF000		
	orr	w3, w3, w0			
	ldr	x0, =OFFSET_4KB
	ldr	x2, [sp, #MYTEE_SP_OFFSET_PAGE_INDEX]
	
static_table_4kb_unshield:
	str	w3, [x1]		
	dc 	cvac, x1		 
	add	w3, w3, #OFFSET_4KB
	add	w0, w0, #0x8
	subs	w2, w2, #1
	bne	static_table_4kb_unshield
	tlbi	alle1
	ldr	x0, [sp, #MYTEE_SP_OFFSET_STATIC_TABLE_SAVED_ADDR]
	ldr	w1, =0x0
	str	w1, [x0]
	sub	w0, w0, #0x4
	str	w1, [x0]
	dc 	cvac, x0
	b	exit4
	
non_static_page_shield:
	ldr	x2, =0x0
	str	w2, [x0]				// Clear mmio_device flag
	dc 	cvac, x0		         
	ldr	x1, [sp, #MYTEE_SP_OFFSET_MMIO_PHYS_ADDR]
	
	// r3 = page index
	// r1 = mmio addr
	
	ldr	x0, =0xFFFFFFFFFFFFFFFF
	
	mov	w2, #0
	orr	w2, w2, 512	// 4KB * 512 = 2MB

	ldr	w1, =OFFSET_4KB
	mul	w3, w3, w1
	mov	w1, #0
	ldr	w1, =MYTEE_HYP_DYNAMICALLY_ALLOCATED_STAGE2_PAGE_TABLE_4KB_PHYS 
	add	w1, w1, w3	// r0 = 0x360007ff, r1=page addr, r2=512, r3 = page index

	mov	w3, #0
	sub	w1, w1, #0x8
loop3:	
	add	w1, w1, #0x8	 
	str	x0, [x1]
	dc 	cvac, x1
		 
	subs 	w2, w2, #1
	bne	loop3
	
	ldr	x1, [sp, #MYTEE_SP_OFFSET_MMIO_PHYS_ADDR]
	
	// change 4mb table to 4mb table
	ldr	w0, =0x3FE00000
	and	w0, w0, w1	
	mov	w2, w1		
	lsr	w0, w0, 0x14		
	ldr	w1, =0x4
	mul	w0, w0, w1		
	
	ldr	w1, =MYTEE_STAGE2_PAGE_TABLE_LEVEL1_1GB_PHYS	
	add	w0, w0, w1		
	
	mov	w2, #0		
	ldr	w2, =STAGE2_TRANSLATION_LEVEL012_LOWER_ATTRIBUTE_ACCESS_PERMISSION_TO_RW
	
	ldr	x3, [sp, #MYTEE_SP_OFFSET_MMIO_PHYS_ADDR]
		
	and	w3, w3, #0x3FE00000		
	orr	w3, w3, w2		
	
					
	str	w3, [x0]		 // r0 = 0x0f0f1d80 => 0x0f0f7003 + (page index * 0x1000) = 4kb page address
	dc 	cvac, x0		 // Clean single data cache line

	tlbi alle1
exit4:	
	add	sp, sp, #(8 * 10)
	ldr     x30, [sp, #CTX_GPREGS_OFFSET + CTX_GPREG_LR]
	eret
.LTORG

18:	cmp	w0, #MYTEE_DMA_SET_NON_CACHEABLE
	bne	19f
	ldr	w0, =0x63c0
	ldr	w1, =0x0f000705
	str	w1, [x0]
	dc 	cvac, x0
	tlbi alle2
	eret	
.LTORG

19:	cmp	w0, #MYTEE_SHIELD_MMIO_WITH_PHYS
	bne	20f
	
	sub	sp, sp, #(8 * 10)
    	
	str	x1, [sp, #MYTEE_SP_OFFSET_MMIO_PHYS_ADDR]	// sp = mmio addr(phys), sp+0x8 = size, sp+0x10 = page index
	str	x3, [sp, #MYTEE_SP_OFFSET_SHIELD_SIZE]
	
	mov	w0, #0					
calc_page_num4:						
	add	w0, w0, #1
	subs	w3, w3, OFFSET_2MB
	cmp	w3, #0x0
	bgt	calc_page_num4
	
	mov	w3, w0
	str	x3, [sp, #MYTEE_SP_OFFSET_SHIELD_PAGE_NUM]
	// change mmio momory table entry(2MB) to permission with No Access
	ldr	w0, =0x3FE00000
	and	w0, w0, w1	
	mov	w3, w0
	lsr	w0, w0, 0x14		
	ldr	w1, =0x4
	mul	w0, w0, w1		
	
	ldr	w1, =MYTEE_STAGE2_PAGE_TABLE_LEVEL1_1GB_PHYS	
	add	w0, w0, w1		
	
	// Check block entry, if it is table entry (last bits are 0b11)
	ldr	w1, [x0]
	and	w1, w1, 0x00000003	
	cmp	w1, #0x3
	beq	exit5
		
	mov	w2, #0		
	ldr	w2, =STAGE2_TRANSLATION_LEVEL012_LOWER_ATTRIBUTE_ACCESS_PERMISSION_TO_NA

	ldr	x1, [sp, #MYTEE_SP_OFFSET_SHIELD_PAGE_NUM]	
	orr	w3, w3, w2		
	ldr	w2, =OFFSET_2MB
	
shield4:
	str	w3, [x0]		
	dc 	cvac, x0		 
	add	w3, w3, w2
	add	w0, w0, #0x8
	subs	w1, w1, #1

	bne	shield4

	tlbi alle1
exit5:
	add	sp, sp, #(8 * 10)
	ldr     x30, [sp, #CTX_GPREGS_OFFSET + CTX_GPREG_LR]
	eret
.LTORG

20:	cmp	w0, #MYTEE_UNSHIELD_MMIO_WITH_PHYS
	bne	21f

	sub	sp, sp, #(8 * 10)
    	
	str	x1, [sp, #MYTEE_SP_OFFSET_MMIO_PHYS_ADDR]
	str	x3, [sp, #MYTEE_SP_OFFSET_SHIELD_SIZE]
	
	mov	w0, #0
					
calc_page_num5:		
	add	w0, w0, #1
	subs	w3, w3, OFFSET_2MB
	cmp	w3, #0x0
	bgt	calc_page_num5
	
	mov	w3, w0
	str	x3, [sp, #MYTEE_SP_OFFSET_SHIELD_PAGE_NUM]
	
	ldr	w0, =0x3FE00000
	and	w0, w0, w1	
	mov	w3, w0
	lsr	w0, w0, 0x14		
	ldr	w1, =0x4
	mul	w0, w0, w1		
	
	ldr	w1, =MYTEE_STAGE2_PAGE_TABLE_LEVEL1_1GB_PHYS	
	add	w0, w0, w1		
	
	// Check block entry, if it is table entry (last bits are 0b11)
	ldr	w1, [x0]
	and	w1, w1, 0x00000003	
	cmp	w1, #0x3
	beq	exit6
		
	mov	w2, #0		// 0x7fd: R/W
	ldr	w2, =0x07fd

	ldr	x1, [sp, #MYTEE_SP_OFFSET_SHIELD_PAGE_NUM]	
	orr	w3, w3, w2		
	ldr	w2, =OFFSET_2MB
	
unshield2:
	str	w3, [x0]		
	dc 	cvac, x0		 
	add	w3, w3, w2
	add	w0, w0, #0x8
	subs	w1, w1, #1	

	bne	unshield2
	
	tlbi alle1
exit6:
	add	sp, sp, #(8 * 10)
	ldr     x30, [sp, #CTX_GPREGS_OFFSET + CTX_GPREG_LR]
	eret
.LTORG
	
21: 	cmp	w0, #MYTEE_GET_KERNEL_PHYS
	bne	22f
	at	s1e1w, x1
    	mrs	x30, par_el1
    	mov	x0, x30
    	and	x0, x0, #0x1
    	cmp	x0, #0x1
    	beq	exit7		// Address translation aborted
    	and	x30, x30, 0xfffffffff000
	and	x1, x1, 0xfff
    	add	x1, x1, x30
    	mov	x0, x1
    	ldr     x30, [sp, #CTX_GPREGS_OFFSET + CTX_GPREG_LR]
	eret
exit7:
	mov	x0, #0x0
	eret
	
22:	cmp	w0, #MYTEE_COPY_CONTROL_BLOCKS_IN_USER
	bne	99f
	// x2 = virt of temp secure buffer, x4 = phys of first cb, [x1, x3] is reserved
	sub	sp, sp, #(8 * 2)
	str	x1, [sp]
	str	x2, [sp, #0x8]
	
	// To avoid infinite loop, check that next CB address does not point to one of the previous CB address
	ldr	x1, =MYTEE_CONTROL_BLOCKS_LOG_PAGE
	str	x4, [x1]
	dc 	cvac, x1
	
	// Get phys of secure buffer[x2]
	at	s1e1w, x2
    	mrs	x30, par_el1
    	mov	x0, x30
    	and	x0, x0, #0x1
    	cmp	x0, #0x1
    	beq	copy_end		// Address translation aborted
    	and	x30, x30, 0xfffffffff000
	and	x2, x2, 0xfff
    	add	x2, x2, x30
	
	// start copy_cb	
	ldr	x1, =0x8
copy_cb:
	ldr	w0, [x4]
	str	w0, [x2]
	add	w4, w4, #0x4
	add	w2, w2, #0x4
	subs	w1, w1, #0x1
	bne	copy_cb
	
	sub	x2, x2, #0x20
	dc 	cvac, x2		 // Clean single data cache line
	add	x2, x2, #0x20
	
	sub	w4, w4, #0xc
	ldr	w4, [x4]
	cmp	w4, #0x0
	beq	copy_end
	
	// convert bus address of CB in user space to the physical address
	mov	x30, x4
	lsr	x30, x30, #28
	sub	x30, x30, #8
	lsl	x30, x30, #28
	and	w4, w4, 0x0fffffff
	orr	x4, x4, x30
    	
	ldr	x30, =MYTEE_CONTROL_BLOCKS_LOG_PAGE
check_infinite_loop:
	ldr	x1, [x30]
	cmp	x1, #0x0
	beq	next_copy
	cmp	x1, x4
	beq	is_infinite_loop	// If the next CB address point to one of the previous CB addresss, to avoid infinite loop, change next block address to 0x0
	add	x30, x30, #0x8
	b	check_infinite_loop

next_copy:	
	// Save next CB address in the log page
	str	x4, [x30]
	dc 	cvac, x30
	    	
	// Start copying next control block
   	ldr	w1, =0x8
	b	copy_cb

is_infinite_loop:
	// Reject DMA request with the infinite loop of CBs
	sub	x2, x2, #0xc
	ldr	x4, =0x0
	str	w4, [x2]
	dc 	cvac, x2
		
copy_end:
	// clear log page
	ldr	x1, =MYTEE_CONTROL_BLOCKS_LOG_PAGE
clear_log_page:	
	ldr	x2, [x1]
	cmp	x2, #0x0
	beq	exit_copy
	ldr	x30, =0x0
	str	x30, [x1]
	dc 	cvac, x1
	add	x1, x1, #0x8
	b	clear_log_page
	
exit_copy:
	ldr	x1, [sp]
	ldr	x2, [sp, #0x8]	
	add	sp, sp, #(8 * 2)
	ldr     x30, [sp, #CTX_GPREGS_OFFSET + CTX_GPREG_LR]
	eret
99:
#endif
	/* Check whether aarch32 issued an SMC64 */
	tbnz	x0, #FUNCID_CC_SHIFT, smc_prohibited

smc_handler64:
	/*
	 * Populate the parameters for the SMC handler.
	 * We already have x0-x4 in place. x5 will point to a cookie (not used
	 * now). x6 will point to the context structure (SP_EL3) and x7 will
	 * contain flags we need to pass to the handler.
	 *
	 * Save x4-x29 and sp_el0.
	 */
	stp	x4, x5, [sp, #CTX_GPREGS_OFFSET + CTX_GPREG_X4]
	stp	x6, x7, [sp, #CTX_GPREGS_OFFSET + CTX_GPREG_X6]
	stp	x8, x9, [sp, #CTX_GPREGS_OFFSET + CTX_GPREG_X8]
	stp	x10, x11, [sp, #CTX_GPREGS_OFFSET + CTX_GPREG_X10]
	stp	x12, x13, [sp, #CTX_GPREGS_OFFSET + CTX_GPREG_X12]
	stp	x14, x15, [sp, #CTX_GPREGS_OFFSET + CTX_GPREG_X14]
	stp	x16, x17, [sp, #CTX_GPREGS_OFFSET + CTX_GPREG_X16]
	stp	x18, x19, [sp, #CTX_GPREGS_OFFSET + CTX_GPREG_X18]
	stp	x20, x21, [sp, #CTX_GPREGS_OFFSET + CTX_GPREG_X20]
	stp	x22, x23, [sp, #CTX_GPREGS_OFFSET + CTX_GPREG_X22]
	stp	x24, x25, [sp, #CTX_GPREGS_OFFSET + CTX_GPREG_X24]
	stp	x26, x27, [sp, #CTX_GPREGS_OFFSET + CTX_GPREG_X26]
	stp	x28, x29, [sp, #CTX_GPREGS_OFFSET + CTX_GPREG_X28]
	mrs	x18, sp_el0
	str	x18, [sp, #CTX_GPREGS_OFFSET + CTX_GPREG_SP_EL0]

	mov	x5, xzr
	mov	x6, sp

#if SMCCC_MAJOR_VERSION == 1

	/* Get the unique owning entity number */
	ubfx	x16, x0, #FUNCID_OEN_SHIFT, #FUNCID_OEN_WIDTH
	ubfx	x15, x0, #FUNCID_TYPE_SHIFT, #FUNCID_TYPE_WIDTH
	orr	x16, x16, x15, lsl #FUNCID_OEN_WIDTH

	load_rt_svc_desc_pointer

#elif SMCCC_MAJOR_VERSION == 2

	/* Bit 31 must be set */
	tbz	x0, #FUNCID_TYPE_SHIFT, smc_unknown

	/*
	 * Check MSB of namespace to decide between compatibility/vendor and
	 * SPCI/SPRT
	 */
	tbz	x0, #(FUNCID_NAMESPACE_SHIFT + 1), compat_or_vendor

	/* Namespaces SPRT and SPCI currently unimplemented */
	b	smc_unknown

compat_or_vendor:

	/* Namespace is b'00 (compatibility) or b'01 (vendor) */

	/*
	 * Add the LSB of the namespace (bit [28]) to the OEN [27:24] to create
	 * a 5-bit index into the rt_svc_descs_indices array.
	 *
	 * The low 16 entries of the rt_svc_descs_indices array correspond to
	 * OENs of the compatibility namespace and the top 16 entries of the
	 * array are assigned to the vendor namespace descriptor.
	 */
	ubfx	x16, x0, #FUNCID_OEN_SHIFT, #(FUNCID_OEN_WIDTH + 1)

	load_rt_svc_desc_pointer

#endif /* SMCCC_MAJOR_VERSION */

	/*
	 * Restore the saved C runtime stack value which will become the new
	 * SP_EL0 i.e. EL3 runtime stack. It was saved in the 'cpu_context'
	 * structure prior to the last ERET from EL3.
	 */
	ldr	x12, [x6, #CTX_EL3STATE_OFFSET + CTX_RUNTIME_SP]

	/* Switch to SP_EL0 */
	msr	spsel, #0

	/*
	 * Save the SPSR_EL3, ELR_EL3, & SCR_EL3 in case there is a world
	 * switch during SMC handling.
	 * TODO: Revisit if all system registers can be saved later.
	 */
	mrs	x16, spsr_el3
	mrs	x17, elr_el3
	mrs	x18, scr_el3
	stp	x16, x17, [x6, #CTX_EL3STATE_OFFSET + CTX_SPSR_EL3]
	str	x18, [x6, #CTX_EL3STATE_OFFSET + CTX_SCR_EL3]

	/* Copy SCR_EL3.NS bit to the flag to indicate caller's security */
	bfi	x7, x18, #0, #1

	mov	sp, x12

	/*
	 * Call the Secure Monitor Call handler and then drop directly into
	 * el3_exit() which will program any remaining architectural state
	 * prior to issuing the ERET to the desired lower EL.
	 */
#if DEBUG
	cbz	x15, rt_svc_fw_critical_error
#endif
	blr	x15

	b	el3_exit


smc_unknown:
	/*
	 * Unknown SMC call. Populate return value with SMC_UNK, restore
	 * GP registers, and return to caller.
	 */
	mov	x0, #SMC_UNK
	str	x0, [sp, #CTX_GPREGS_OFFSET + CTX_GPREG_X0]
	b	restore_gp_registers_eret

smc_prohibited:
	ldr	x30, [sp, #CTX_GPREGS_OFFSET + CTX_GPREG_LR]
	mov	x0, #SMC_UNK
	eret

rt_svc_fw_critical_error:
	/* Switch to SP_ELx */
	msr	spsel, #1
	no_ret	report_unhandled_exception
endfunc smc_handler

// phy_addr: x24, context: x25
func verify_pg_dst 

	mov x29, #0		//Init the return value with no-err
	
	check_mytee_init_flag
	cmp w26, #1
        bne 45f		      

	and x24, x24, #0xffffffff
	/** Common verification for all invokers **/
	
	// Check if it's swapper pgd. Do not update it.
        and x26, x24, #0xfffff000
        mov x27, #SWAPPER_PGD_ADDR
        cmp x26, x27
        beq 44f

        // Check if the phy_addr falls within the protected region
        mov x27, #HYP_START_ADDR
        cmp x24, x27
        blt 1f
        mov x27, #TEE_END_ADDR
        cmp x24, x27
        blt 44f                                 // Map to hyp or TEE

1:
        mov x27, #KERNEL_TEXT_START_ADDR
        cmp x24, x27
        blt 2f
        mov x27, #KERNEL_TEXT_END_ADDR
        cmp x24, x27
        blt 44f                                 // Map to kernel text
	
2:
	// Check the attempts to manipulate the protected region
        mov w26, #BLANK_PAGE_MAP_ADDR
        ldr w26, [x26, #PGT_BITMAP_ADDR_OFFSET]
	
	/** (Verification depending on invoker type) **/
	lsr x27, x24, #12		// Get the index (pfn)
	add w27, w27, w26
	ldrb w27, [x27]			// Get the bitmap flag
	
	// ( ... invoker dependant verification ...)
	mov x28, x25 
        cmp x28, #EMUL_MEM_PTE
        bne 1f

        mov w26, #MYTEE_ROBUF_START
        cmp x24, x26
        blt 3f
        add w26, w26, #MYTEE_ROBUF_SIZE
        cmp x24, x26
        blt 44f
3:		// Assume that the pte is already resitered in the bitmap by the PMD-related routines
        and w26, w27, #0xf
        cmp w26, #MYTEE_SWP_PTE                     //if SWP pte
        beq 45f
        cmp w26, #MYTEE_APP_PTE                     //if APP pte
        beq 45f
        // TODO: Add more verification...
	b 45f                           //Something wrong
1:		// Assume the copies from the swp's to the app's
        cmp x28, #EMUL_COPY_PMD
        bne 1f

        mov w26, #MYTEE_ROBUF_START
        cmp x24, x26
        blt 44f
        add w26, w26, #MYTEE_ROBUF_SIZE
        cmp x24, x26
        bge 44f

	and w26, w27, #0xf
	cmp w26, #MYTEE_APP_PMD
	bne 44f

        b 45f                           // Pass all

1:
        cmp x28, #EMUL_PMD_CLEAR
        bne 1f

        mov w26, #MYTEE_ROBUF_START
        cmp x24, x26
        blt 3f
        add w26, w26, #MYTEE_ROBUF_SIZE
        cmp x24, x26
        blt 45f
3:
        and x26, x24, #0xfffff000
        cmp x26, #0x6000            //SWP pmd
        beq 45f
        cmp x26, #0x7000            //SWP pmd
        beq 45f

        b 44f                           // Something worng

1:
        cmp x28, #EMUL_PMD_FREE
        bne 1f

        and w26, w27, #0xf
        cmp w26, #2         //SWP pmd
        beq 45f
        cmp w26, #6         //APP pmd
        beq 45f

        b 44f                           // Something wrong

1:
        cmp x28, #EMUL_SET_PUD
        bne 1f

        mov w26, #MYTEE_ROBUF_START
        cmp x24, x26
        blt 44f
        add w26, w26, #MYTEE_ROBUF_SIZE
        cmp x24, x26
        bge 44f

        b 45f                           // Pass all
1:
        cmp x28, #EMUL_PUD_CLEAR
        bne 1f

        mov w26, #MYTEE_ROBUF_START
        cmp x24, x26
        blt 44f
        add w26, w26, #MYTEE_ROBUF_SIZE
        cmp x24, x26
        bge 44f

        b 45f                           // Pass all
1:
        //We do not check PGD free
        cmp x28, #EMUL_PGD_FREE
        bne 45f

        b 45f                           // Pass all

44:
        mov x29, #1                    // Something wrong 
45:
        ret
endfunc verify_pg_dst

// value_low: x24, context: x25
func verify_pg_value 

	mov x29, #0		//Init the return with no-err

        check_mytee_init_flag
        cmp w26, #1
        bne 45f

	and x24, x24, #0xffffffff
	 /** Common verification for all invokers **/

        // Check if it's swapper pgd. Do not update it.
        and x26, x24, #0xfffff000
	mov x27, #SWAPPER_PGD_ADDR
        cmp x26, x27
        beq 44f

        // Check if the phy_addr falls within the protected region
	mov x27, #TEE_START_ADDR
        cmp x24, x27
        blt 1f
	mov x27, #TEE_END_ADDR
        cmp x24, x27
        blt 44f

1:      
        mov x27, #KERNEL_TEXT_START_ADDR
	cmp x24, x27
        blt 2f
	mov x27, #KERNEL_TEXT_END_ADDR
        cmp x24, x27
        bge 2f                                 // Map to kernel text
        tst x24, 0x40			// Check if it's mapping to EL0
        bne 44f				// Writable mapping, it's error
2:
        // Check the case that the value points to ROBUFs
        mov w26, #BLANK_PAGE_MAP_ADDR
        ldr w26, [x26, #PGT_BITMAP_ADDR_OFFSET]
        
        lsr x27, x24, #12         // Get the index (pfn)
        add w27, w27, w26
        ldrb w27, [x27]                 // Get the bitmap flag

        // ( ... invoker dependant verification ...)
	mov x28, x25
1:
    	cmp x28, #EMUL_COPY_PMD
    	bne 1f

	b 45f                           // Pass all
1:
    	cmp x28, #EMUL_PMD_CLEAR
    	bne 1f

        b 45f
1:
    	cmp x28, #EMUL_PMD_FREE
    	bne 1f
	
	// no param for pmd_free
	b 45f

1:
    	cmp x28, #EMUL_SET_PUD
    	bne 1f
		
	mov w26, #MYTEE_ROBUF_START
        cmp x24, x26
        blt 44f
        add w26, w26, #MYTEE_ROBUF_SIZE
        cmp x24, x26
        bge 44f

        b 45f                           // Pass all
1:
    	cmp x28, #EMUL_PUD_CLEAR
    	bne 1f
	// No param for pud_clear
	// We don't care DoS attack
        b 45f                           // Pass all
1:
	//We do not check PGD free, no param
    	cmp x28, #EMUL_PGD_FREE

	b 45f                           // Pass all

44:
        mov x29, #1                     // Error
45:
	ret	
endfunc verify_pg_value

/** set(clear)_pgt_bitmap assumes that the input values are already verified **/
// x24: phy_addr, x25: map_type
func set_pgt_bitmap
	
        check_mytee_init_flag
        cmp w26, #1
        bne 45f

	and x24, x24, #0xffffffff

	mov w26, #BLANK_PAGE_MAP_ADDR
1:
        ldr w27, [x26, #PGT_BITMAP_SPINLOCK_OFFSET]     // Spinlock
        cmp w27, #1 
        beq 1b
        mov w27, #1
        str w27, [x26, #PGT_BITMAP_SPINLOCK_OFFSET]                     // Lock
        dc cvac, x26

	ldr w26, [x26, #PGT_BITMAP_ADDR_OFFSET]		     // Get the pgt_bitmap address
	lsr x27, x24, #12     // Div by a page (0x1000)
	add w27, w27, w26	    // Get bitmap address (1 byte per each page)
	ldrb w28, [x27]		    // Get the bitmap flag for the input phy_addr
	lsr w26, w28, #4
	and w26, w26, #0xf
	add w26, w26, #1
	lsl w26, w26, #4
	mov x28, x25
	and x28, x28, #0xf
	orr x26, x26, x28
	strb w26, [x27]	
	dc cvac, x27

	mov w28, #BLANK_PAGE_MAP_ADDR
        str wzr, [x28, #PGT_BITMAP_SPINLOCK_OFFSET]                  // Lock release
        dsb sy
45:
	ret
endfunc set_pgt_bitmap

// x24: phy_addr
func clear_pgt_bitmap 

        check_mytee_init_flag
        cmp w26, #1
        bne 45f

	and x24, x24, #0xffffffff

        mov w26, #BLANK_PAGE_MAP_ADDR
1:
        ldr w27, [x26, #PGT_BITMAP_SPINLOCK_OFFSET]     // Spinlock
        cmp w27, #1 
        beq 1b
        mov w27, #1
        str w27, [x26, #PGT_BITMAP_SPINLOCK_OFFSET]                     // Lock
        dc cvac, x26

	ldr w26, [x26, #PGT_BITMAP_ADDR_OFFSET]	   // Get the pgt_bitmap address	
        lsr x27, x24, #12     	    // Div by a page (0x1000)
        add w27, w27, w26           // Get bitmap address (1 byte per each page)
        ldrb w28, [x27]             // Get the bitmap flag for the input phy_addr
        and w26, w26, #0xf	    // Get ref counter value	
	cmp w26, #0 
	beq 1f              
	sub x26, x26, #1 	    // Sub 1, if greater than 0
	cmp w26, #0
	beq 2f			    // Check if some references still exist		    	
	lsl w26, w26, #4	   
	and w28, w28, #0xf	   	
	orr w26, w26, w28	    // Keep the current type value, if ref > 0	
        strb w26, [x27] 
	dc cvac, x27
	b 1f

2:
	strb wzr, [x27]
	dc cvac, x27

1:
        mov w28, #BLANK_PAGE_MAP_ADDR
        str wzr, [x28, #PGT_BITMAP_SPINLOCK_OFFSET]                  // Lock release
        dsb sy

45:
	ret
endfunc clear_pgt_bitmap

