/*
 * Copyright (c) 2012 Linaro Limited.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <linux/init.h>
#include <linux/irqchip/arm-gic-v3.h>
#include <linux/linkage.h>
#include <asm/assembler.h>
#include <asm/virt.h>

#ifdef CONFIG_MYTEE
#include <asm/kvm_asm.h>
#include <asm/kvm_arm.h>
#include <asm/kvm_mmu.h>
#define MYTEE_SP_OFFSET_CONTEXT_X0 0x0
#define MYTEE_SP_OFFSET_CONTEXT_X1 0x4
#define MYTEE_SP_OFFSET_CONTEXT_X2 0x8
#define MYTEE_SP_OFFSET_CONTEXT_X3 0xc
#define MYTEE_SP_OFFSET_CONTEXT_X4 0x10
#define MYTEE_SP_OFFSET_CONTEXT_X5 0x14
#define MYTEE_SP_OFFSET_CONTEXT_X6 0x18
#define MYTEE_SP_OFFSET_CONTEXT_X7 0x1c
#define MYTEE_SP_OFFSET_CONTEXT_X8 0x20
#define MYTEE_SP_OFFSET_CONTEXT_X9 0x24
#define MYTEE_SP_OFFSET_CONTEXT_X10 0x28
#define MYTEE_SP_OFFSET_CONTEXT_X11 0x2c
#define MYTEE_SP_OFFSET_CONTEXT_X12 0x30
#define MYTEE_SP_OFFSET_CONTEXT_X13 0x34
#define MYTEE_SP_OFFSET_CONTEXT_X14 0x38
#define MYTEE_SP_OFFSET_DMA_SECURE_BUFFER 0x3c
#define MYTEE_TEMP_SAVE_CONTEXT_X0 0x40
#define MYTEE_TEMP_SAVE_CONTEXT_X1 0x44
#define MYTEE_TEMP_SAVE_CONTEXT_X2 0x48
#define MYTEE_TEMP_SAVE_CONTEXT_X3 0x4c

#define MYTEE_STAGE2_PAGE_TABLE_BASE_PHYS 0x0F0F0000
#define STAGE2_TRANSLATION_LEVEL012_LOWER_ATTRIBUTE_ACCESS_PERMISSION_TO_RW 0x7FD
#define MYTEE_HYP_STACK_BASE 0x0F10E000

#define KERNEL_TEXT_PHYS_START 0x00200000
#define KERNEL_TEXT_PHYS_END 0x00A00000

#define HYP_PHYS_START 0x0E800000
#define HYP_PHYS_END 0x100E0000
#define EL3_PHYS_START 0x100E0000
#define EL3_PHYS_END 0X10100000
#define OPTEE_PHYS_START 0x10100000
#define OPTEE_PHYS_END 0x11000000

#define HYP_EL3_OPTEE_PHYS_START 0x0E800000
#define HYP_EL3_OPTEE_PHYS_END 0x11000000

#define DMA_VIRT_ADDR 0xBB80D000
#define SECURE_BUFFER_BASE_VIRT 0x8F1FB000
#define BCM2837_BUS_VIRT_OFFSET 0x40000000
#define BCM2837_BUS_PHYS_OFFSET 0xC0000000
#define BCM2837_USER_BUS_TO_PHYS_OFFSET 0x80000000
#define MYTEE_CONTROL_BLOCKS_LOG_PAGE 0x800000
#define MYTEE_DEVNUM_SAVE_VIRT 0x8F102000
#define MYTEE_HC_CHAR_SAVE_VIRT 0X8F102010
#define MYTEE_HC_NUM_SAVE_VIRT 0x8F102020
#define MYTEE_SECURE_KBD_FLAG_VIRT 0x8F102030
#define MYTEE_USBHID_INBUF_DMA_VIRT 0x8F102060
#define MYTEE_HOST_CONTROLLER_REGS_VIRT 0xF09805
#define MYTEE_SECURE_USBHID_INBUF_DMA_BUS 0xCF100000
#define MYTEE_BCM2835_IO_DEVICE_ADDRESS_BUS 0x7E000000
#define MYTEE_USER_SPACE_ADDRESS_BUS 0xB0000000
#define MYTEE_DMA_MMIO_ADDRESS_PHYS 0x3F007000
#define MYTEE_PHYS_TO_VIRT_OFFSET 0x80000000
#define MYTEE_TTBR1_BASE_PHYS 0x3000
#define VALUE_REQUEST_LED 0x2000921
#define OFFSET_4KB 0x1000

#define MYTEE_COPY_CONTROL_BLOCKS_IN_USER 116
#endif

#ifndef ZIMAGE
/*
 * For the kernel proper, we need to find out the CPU boot mode long after
 * boot, so we need to store it in a writable variable.
 *
 * This is not in .bss, because we set it sufficiently early that the boot-time
 * zeroing of .bss would clobber it.
 */
.data
	.align	2

ENTRY(__boot_cpu_mode)
	.long	0
.text
	/*
	 * Save the primary CPU boot mode. Requires 3 scratch registers.
	 */
	.macro	store_primary_cpu_mode	reg1, reg2, reg3
	mrs	\reg1, cpsr
	and	\reg1, \reg1, #MODE_MASK
	adr	\reg2, .L__boot_cpu_mode_offset
	ldr	\reg3, [\reg2]
	str	\reg1, [\reg2, \reg3]
	.endm

	/*
	 * Compare the current mode with the one saved on the primary CPU.
	 * If they don't match, record that fact. The Z bit indicates
	 * if there's a match or not.
	 * Requires 3 additionnal scratch registers.
	 */
	.macro	compare_cpu_mode_with_primary mode, reg1, reg2, reg3
	adr	\reg2, .L__boot_cpu_mode_offset
	ldr	\reg3, [\reg2]
	ldr	\reg1, [\reg2, \reg3]
	cmp	\mode, \reg1		@ matches primary CPU boot mode?
	orrne	\reg1, \reg1, #BOOT_CPU_MODE_MISMATCH
	strne	\reg1, [\reg2, \reg3]	@ record what happened and give up
	.endm

#else	/* ZIMAGE */

	.macro	store_primary_cpu_mode	reg1:req, reg2:req, reg3:req
	.endm

/*
 * The zImage loader only runs on one CPU, so we don't bother with mult-CPU
 * consistency checking:
 */
	.macro	compare_cpu_mode_with_primary mode, reg1, reg2, reg3
	cmp	\mode, \mode
	.endm

#endif /* ZIMAGE */

#ifndef ZIMAGE
.align 2
.L__boot_cpu_mode_offset:
	.long	__boot_cpu_mode - .
#endif

.align 5
ENTRY(__hyp_stub_vectors)
__hyp_stub_reset:	W(b)	.
__hyp_stub_und:		W(b)	.
__hyp_stub_svc:		W(b)	.
__hyp_stub_pabort:	W(b)	.
__hyp_stub_dabort:	W(b)	.
__hyp_stub_trap:	W(b)	__hyp_stub_do_trap
__hyp_stub_irq:		W(b)	.
__hyp_stub_fiq:		W(b)	.
ENDPROC(__hyp_stub_vectors)
.align 2

#ifdef CONFIG_MYTEE
/*	
 *	createS2TableBlockEntry
 * 	Input: 
 *	r8: page address
 *	r10: start physical address for mapping
 *  	r7: Size of block (1GB, 2MB)
 *  	r9: Number of entries to create
*/
.macro createS2TableBlockEntry pgtAddr, startPhy, size, numEntry
//Fill L1 desc for 1GB Block mapping    
    mov r12, #STAGE2_TRANSLATION_LEVEL012_LOWER_ATTRIBUTE_ACCESS_PERMISSION_TO_RW
    add \startPhy, r12       //First 1GB mapping (0-0x3FFFFFFF)
    mov r11, #0x0
11:
    str \startPhy, [\pgtAddr]
    add \pgtAddr, #0x4
    str r11, [r8]	
    sub \numEntry, #1
    cmp \numEntry, #0
    add \startPhy, r7
    add \pgtAddr, #0x4
    bgt 11b
.endm
#endif

/*
 * Hypervisor stub installation functions.
 *
 * These must be called with the MMU and D-cache off.
 * They are not ABI compliant and are only intended to be called from the kernel
 * entry points in head.S.
 */
@ Call this from the primary CPU
ENTRY(__hyp_stub_install)
	store_primary_cpu_mode	r4, r5, r6
ENDPROC(__hyp_stub_install)
	@ fall through...
@ Secondary CPUs should call here
ENTRY(__hyp_stub_install_secondary)
	mrs	r4, cpsr
	and	r4, r4, #MODE_MASK

	/*
	 * If the secondary has booted with a different mode, give up
	 * immediately.
	 */
	compare_cpu_mode_with_primary	r4, r5, r6, r7
	retne	lr

	/*
	 * Once we have given up on one CPU, we do not try to install the
	 * stub hypervisor on the remaining ones: because the saved boot mode
	 * is modified, it can't compare equal to the CPSR mode field any
	 * more.
	 *
	 * Otherwise...
	 */

	cmp	r4, #HYP_MODE
	retne	lr			@ give up if the CPU is not in HYP mode

/*
 * Configure HSCTLR to set correct exception endianness/instruction set
 * state etc.
 * Turn off all traps
 * Eventually, CPU-specific code might be needed -- assume not for now
 *
 * This code relies on the "eret" instruction to synchronize the
 * various coprocessor accesses. This is done when we switch to SVC
 * (see safe_svcmode_maskall).
 */
	@ Now install the hypervisor stub:
	W(adr)	r7, __hyp_stub_vectors
	mcr	p15, 4, r7, c12, c0, 0	@ set hypervisor vector base (HVBAR)


	@ Disable all traps, so we don't get any nasty surprise
#ifdef CONFIG_MYTEE
/* Setup the secondary page table
 * Page directory location: 0x0F0F0000
 * The page table is created by arm-trusted-firmware (plat/rpi/rpi_common.c)
 * TODO: nullify the hyp-related instructions by active monitor at boot time
 */

 	.equ label, 0x0                @high of vttbr  : vmid==0
	movw r8, #:lower16:label
	movt r8, #:upper16:label  	

 	.equ label, MYTEE_STAGE2_PAGE_TABLE_BASE_PHYS             @low of vttbr for n.w.
	movw r7, #:lower16:label
	movt r7, #:upper16:label
	mcrr p15, 6, r7, r8, c2     	    @set VTTBR
	
//	.equ label, 0x3550		@inner sharable(3), normal-outer&inner-write-back-write-allocate(5), start at first level& sign extension(5)
	.equ label, 0x2a50		@outher sharable(2), normal-outer&inner-write-through(a)
	movw r8, #:lower16:label
	movt r8, #:upper16:label  	
        mcr p15, 4, r8, c2, c1, 2	@set VTCR

	dsb
	isb
	
	mrc	p15, 0, r0, c0, c0, 5
	and	r0, #0x00000003
	teq	r0, #0x0
	beq	1f
	
	mov	r0, #0
	orr	r0, r0, #0x4000
	orr	r0, r0, #0x08
	mov	r2, #0
	orr	r2, r2, #0x200000
	orr	r2, r2, #0x0700
	orr	r2, r2, #0x1D
	str	r2, [r0]

	mov     r0, #MYTEE_TTBR1_BASE_PHYS             // First entry for TTBR1
        mov     r2, #0x6003             // Make it equivalent to the 3rd entry
        str     r2, [r0]
        dmb
        mcr     p15, 0, r0, c7, c6, 1

	mov	r0, #0
	orr	r0, r0, #MYTEE_TTBR1_BASE_PHYS
	orr	r0, r0, #0x10
	mov	r1, #0
	mcrr	p15, 4, r0, r1, c2

	mov 	r0, #0
	orr 	r0, r0, #0x80000000
	orr 	r0, r0, #0x800000
	orr	r0, r0, #0x3500
        mcr     p15, 4, r0, c2, c0, 2   @ HTCR
	
        @ Use the same memory attributes for hyp. accesses as the kernel
        @ (copy MAIRx ro HMAIRx).

	mov 	r0, #0
	orr	r0, r0, #0xee000000
	orr	r0, r0, #0xaa0000
	orr	r0, r0, #0x4400
        mcr     p15, 4, r0, c10, c2, 0	//HMAIR0
	
	mov	r0, #0
	orr	r0, r0, #0xFF000000
	orr	r0, r0, #0x4
        mcr     p15, 4, r0, c10, c2, 1 //HMAIR1
		
        @ Invalidate the stale TLBs from Bootloader
        mcr     p15, 4, r0, c8, c7, 0   @ TLBIALLH
        dsb     ish

        @ Set the HSCTLR to:
        @  - ARM/THUMB exceptions: Kernel config (Thumb-2 kernel)
        @  - Endianness: Kernel config
        @  - Fast Interrupt Features: Kernel config
        @  - Write permission implies XN: disabled
        @  - Instruction cache: enabled
        @  - Data/Unified cache: enabled
        @  - MMU: enabled (this code must be run from an identity mapping)

	mov 	r0, #0x0
	orr	r0, r0, #0x30000000
	orr	r0, r0, #0xc50000
	orr	r0, r0, #0x1800
	orr	r0, r0, #0x35
        mcr     p15, 4, r0, c1, c0, 0   @ HSCR
	isb	
		
	mrc	p15, 4, r1, c12, c0, 0
	add	r1, r1, #0x80000000			//20210325
	mcr	p15, 4, r1, c12, c0, 0	@ set HVBAR
	instr_sync

/* Enable the secondary paging: SCR.VM=1*/
1:
	mov 	r7, #1
	mcr	p15, 4, r7, c1, c1, 0	@ HCR	
	mov 	r7, #0
	mcr	p15, 4, r7, c1, c1, 2	@ HCPTR
	mcr	p15, 4, r7, c1, c1, 3	@ HSTR
#else
	mov	r7, #0
	mcr	p15, 4, r7, c1, c1, 0	@ HCR
	mcr	p15, 4, r7, c1, c1, 2	@ HCPTR
	mcr	p15, 4, r7, c1, c1, 3	@ HSTR
#endif
	mrc	p15, 4, r7, c1, c1, 1	@ HDCR
	and	r7, #0x1f		@ Preserve HPMN
	mcr	p15, 4, r7, c1, c1, 1	@ HDCR

	@ Make sure NS-SVC is initialised appropriately
	mrc	p15, 0, r7, c1, c0, 0	@ SCTLR
	orr	r7, #(1 << 5)		@ CP15 barriers enabled
	bic	r7, #(3 << 7)		@ Clear SED/ITD for v8 (RES0 for v7)
	bic	r7, #(3 << 19)		@ WXN and UWXN disabled
	mcr	p15, 0, r7, c1, c0, 0	@ SCTLR

	mrc	p15, 0, r7, c0, c0, 0	@ MIDR
	mcr	p15, 4, r7, c0, c0, 0	@ VPIDR

	mrc	p15, 0, r7, c0, c0, 5	@ MPIDR
	mcr	p15, 4, r7, c0, c0, 5	@ VMPIDR

#if !defined(ZIMAGE) && defined(CONFIG_ARM_ARCH_TIMER)
	@ make CNTP_* and CNTPCT accessible from PL1
	mrc	p15, 0, r7, c0, c1, 1	@ ID_PFR1
	lsr	r7, #16
	and	r7, #0xf
	cmp	r7, #1
	bne	1f
	mrc	p15, 4, r7, c14, c1, 0	@ CNTHCTL
	orr	r7, r7, #3		@ PL1PCEN | PL1PCTEN
	mcr	p15, 4, r7, c14, c1, 0	@ CNTHCTL
	mov	r7, #0
	mcrr	p15, 4, r7, r7, c14	@ CNTVOFF

	@ Disable virtual timer in case it was counting
	mrc	p15, 0, r7, c14, c3, 1	@ CNTV_CTL
	bic	r7, #1			@ Clear ENABLE
	mcr	p15, 0, r7, c14, c3, 1	@ CNTV_CTL
1:
#endif

#ifdef CONFIG_ARM_GIC_V3
	@ Check whether GICv3 system registers are available
	mrc	p15, 0, r7, c0, c1, 1	@ ID_PFR1
	ubfx	r7, r7, #28, #4
	cmp	r7, #1
	bne	2f

	@ Enable system register accesses
	mrc	p15, 4, r7, c12, c9, 5	@ ICC_HSRE
	orr	r7, r7, #(ICC_SRE_EL2_ENABLE | ICC_SRE_EL2_SRE)
	mcr	p15, 4, r7, c12, c9, 5	@ ICC_HSRE
	isb

	@ SRE bit could be forced to 0 by firmware.
	@ Check whether it sticks before accessing any other sysreg
	mrc	p15, 4, r7, c12, c9, 5	@ ICC_HSRE
	tst	r7, #ICC_SRE_EL2_SRE
	beq	2f
	mov	r7, #0
	mcr	p15, 4, r7, c12, c11, 0	@ ICH_HCR
2:
#endif

	bx	lr			@ The boot CPU mode is left in r4.
ENDPROC(__hyp_stub_install_secondary)

__hyp_stub_do_trap:

#ifdef CONFIG_MYTEE

// Save general regs r0-r3

mov sp, #0x1000
mrc     p15, 0, lr, c0, c0, 5              @ get processor id:MPIDR
and lr, lr, #0xf
mul sp, sp, lr
ldr lr, =MYTEE_HYP_STACK_BASE		// Temp stack (physical) address. Fixed to 4 KB (further check is needed to prevent overflow)
add sp, lr

str r0, [sp]	
str r1, [sp, #4]
str r2, [sp, #8]
str r3, [sp, #12]
str r4, [sp, #16]
str r5, [sp, #20]
str r6, [sp, #24]

// We first check if the current trap is owing to data abort from OS
// Check if HSR.EC == 0x24 (data abort from OS)
mrc p15, 4, r0, c5, c2, 0 //read hsr register
lsr r2, r0, #26
cmp r2, #0x24 
// Else go to the normal trap handler routine
bne 99f


// Check ISS[6] that indicates the abort reason (0: read access. 1: write access)
tst r0, #0x40	// Test if ISS[6] is 1
bne 2f		// It's 1 (CPSR.Z == 0)
  
// For read access fault (ISS[6)==0)
// Get source address of read
mrc p15, #4, r3, c6, c0, #0		//hdfar
// Validate if it's malicious access to the TEE and hyp


// get the sign extension flag (SSE[21]
mov r2, r0
lsr r2, r2, #21
and r2, r2, #1
// get SAS[23:22] for read access size
mov r1, r0
lsr r1, r1, #22
and r1, #3
b do_read

2:
// For handling write access fault (ISS[6]==1)
// Get source address of the write
mrc p15, #4, r3, c6, c0, #0		//hdfar
// We do not support (implement) SIGN EXTENSION for write
// Get SAS[23:22] for write access size
mov r1, r0
lsr r1, r1, #22
and r1, #3
b do_write

.LTORG 

// Restore r0-r3 and go to the normal routine

99:
ldr r0, [sp]
ldr r1, [sp, #4]
ldr r2, [sp, #8]
ldr r3, [sp, #12]
ldr r4, [sp, #16]
ldr r5, [sp, #20]
ldr r6, [sp, #24]
#endif

1:
	CPSID iaf // Disable interrupts
	teq	r0, #HVC_SET_VECTORS
	bne	1f
	mcr	p15, 4, r1, c12, c0, 0	@ set HVBAR
	b	__hyp_stub_exit

1:	teq	r0, #HVC_SOFT_RESTART
	bne	1f
	bx	r1

1:	teq	r0, #HVC_RESET_VECTORS
#ifdef CONFIG_MYTEE
	bne	1f
#endif	
	beq	__hyp_stub_exit

	ldr	r0, =HVC_STUB_ERR
	__ERET
.LTORG 
#ifdef CONFIG_MYTEE
1:	teq	r0, #MYTEE_SET_VTCR
	bne	1f
	__ERET


// This function can be nullified by active monitor (EL3) at boot time

1:      teq    r0, #MYTEE_LAZY_CPU1_SETUP
	bne	1f
	mov	r0, #MYTEE_TTBR1_BASE_PHYS		// First Entry for TTBR1
	mov	r2, #0x6003		// Make it equivalent to the 3rd entry
	str     r2, [r0]
	dmb
	mcr     p15, 0, r0, c7, c6, 1
	
	mov	r0, #0
	orr	r0, r0, #MYTEE_TTBR1_BASE_PHYS
	orr	r0, r0, #0x10
	mov	r1, #0
	mcrr	p15, 4, r0, r1, c2	@HTTBR

        mrc     p15, 4, r1, c12, c0, 0
        add     r1, r1, #0x80000000
        mcr     p15, 4, r1, c12, c0, 0  @ set HVBAR

	
	mov 	r0, #0
	orr 	r0, r0, #0x80000000
	orr 	r0, r0, #0x800000
	orr	r0, r0, #0x3500
        mcr     p15, 4, r0, c2, c0, 2   @ HTCR
	
        @ Use the same memory attributes for hyp. accesses as the kernel
        @ (copy MAIRx ro HMAIRx).
	
	mov 	r0, #0
	orr	r0, r0, #0xee000000
	orr	r0, r0, #0xaa0000
	orr	r0, r0, #0x4400
        mcr     p15, 4, r0, c10, c2, 0	//HMAIR0
	
	mov	r0, #0
	orr	r0, r0, #0xFF000000
	orr	r0, r0, #0x4
        mcr     p15, 4, r0, c10, c2, 1 //HMAIR1
		
        @ Invalidate the stale TLBs from Bootloader
        mcr     p15, 4, r0, c8, c7, 0   @ TLBIALLH
        dsb     ish

        @ Set the HSCTLR to:
        @  - ARM/THUMB exceptions: Kernel config (Thumb-2 kernel)
        @  - Endianness: Kernel config
        @  - Fast Interrupt Features: Kernel config
        @  - Write permission implies XN: disabled
        @  - Instruction cache: enabled
        @  - Data/Unified cache: enabled
        @  - MMU: enabled (this code must be run from an identity mapping)
	
	mov 	r0, #0x0
	orr	r0, r0, #0x30000000
	orr	r0, r0, #0xc50000
	orr	r0, r0, #0x1800
	orr	r0, r0, #0x35
        mcr     p15, 4, r0, c1, c0, 0   @ HSCR
	isb	

1:	teq	r0, #MYTEE_MEMCOPY
	bne	1f
	mcr	p15, 0, r0, c8, c7, 0	// TLB flush
	isb
	dsb
	mcr	p15, 0, r1, c7, c6, 1	// r1 data invalidate
	mcr	p15, 0, r3, c7, c6, 1	// r3 data invalidate
	
	// r0 = ip
	// r1 = iobuf_phy
	// r2 = transfer_len
	// r3 = in

	cmp	r2, #0
	beq	2f
	mov	r0, r3
loop:
	ldrb	r0, [r1], #1
	strb	r0, [r3], #1
	subs 	r2, r2, #1
	bne	loop
2:	mcr	p15, 0, r0, c8, c7, 0	// TLB flush
	isb
	dsb
	__ERET	


1:	teq	r0, #MYTEE_UP_PRIV
	bne	1f
	
	.arch_extension virt		// Verify if priv_up is called from the static kernel text (not from Loadable Kernel Module)
	mrs	r2, LR_svc
	mcr	p15,0,r2,c7,c8,0	// Get the physical address of return
	mrc	p15,0,r1,c7,c4,0
    	mov	r0, r1
    	and	r0, r0, #0x1
    	cmp	r0, #0x1
    	beq	verify_fail		// Address translation aborted
    	lsr	r1, r1, #12
    	lsl	r1, r1, #12
	lsl	r0, lr, #20
	lsr	r0, r0, #20
    	add	r0, r0, r1
	
	ldr	r1, =KERNEL_TEXT_PHYS_START	// Start verification. Allow [KERNEL_TEXT_PHYS_START =< lr < KERNEL_TEXT_PHYS_END]
	cmp	r1, r0
	bgt	verify_fail			// lr < KERNEL_TEXT_PHYS_START: fail
	ldr	r1, =KERNEL_TEXT_PHYS_END
	cmp	r1, r0
	ble	verify_fail			// KERNEL_TEXT_PHYS_END =< lr: fail
		
	mcr	p15, 4, r0, c8, c7, 0
	mcr	p15, 0, r0, c7, c5, 0 // ICIALLU
	mcr	p15, 0, r0, c7, c1, 0 // BPIALL
	dsb
	isb

	// Return without deprivilging to the kernel mode	
	mrs	r1, LR_svc
	ret 	r1	// return to the location after hvc

verify_fail:
	__ERET
.LTORG

1:	teq	r0, #MYTEE_USBHID_INBUF_DMA_SAVE
	bne	1f
	mov 	r0, #0x0
	ldr	r0, =MYTEE_USBHID_INBUF_DMA_VIRT
check:
	ldr	r2, [r0]
	cmp	r2, #0x0
	beq	save
	add	r0, r0, #0x8
	b	check
save:
	str	r1, [r0]
	__ERET
.LTORG
1:
#endif	

__hyp_stub_exit:
	mov	r0, #0
	__ERET
ENDPROC(__hyp_stub_do_trap)

#ifdef CONFIG_MYTEE

#define concatReg(b) r ## b

/** fix_base_reg_ldr(h) macros fix up the base register values
    to handle the post- and pre-offset ldr(h) instructions 
    that update them with the offset value                      **/

// value: source/target register
// r3: faulting address

.macro fix_base_reg_ldr, value
   // Save the return value (result of ldr)
   .IF \value == 0
	str concatReg(\value), [sp]
	b 33f
   .ELSEIF \value == 1
	str concatReg(\value), [sp, #4]
	b 33f
   .ELSEIF \value == 2
	str concatReg(\value), [sp, #8]
	b 33f
   .ELSEIF \value == 3
	str concatReg(\value), [sp, #12]
	b 33f
   .ELSEIF \value == 4
	str concatReg(\value), [sp, #16]
	b 33f
   .ELSEIF \value == 5
	str concatReg(\value), [sp, #20]
	b 33f
   .ELSEIF \value == 6
	str concatReg(\value), [sp, #24]
	b 33f
   .ELSE	
	b 33f
   .ENDIF
33:
	// Check if the LDR updates the base register
	//// Get the fault instruction 
	mrs r0, ELR_hyp
	ldr r0, [r0]
	//// Decode and check the Write-back bit (W:21)
	lsr r1, r0, #21
	and r1, #1
	//// If W==1, then update the base reg.
	cmp r1, #1
	bne 34f
	//// Get the base reg number
	lsr r1, r0, #16
	and r1, #0xf
	//// Get the fault address that is trapped to the hyp
	//mrc p15, #4, r0, c6, c0, #0             //hdfar	
	cmp r1, #0
	bne 35f
	str r3, [sp]
	b 34f
35:
	cmp r1, #1
	bne 35f
	str r3, [sp, #4]
	b 34f
35:
	cmp r1, #2
	bne 35f
	str r3, [sp, #8]
	b 34f
35:
	cmp r1, #3
	bne 35f
	str r3, [sp, #12]
	b 34f
35:
	cmp r1, #4
	bne 35f
	str r3, [sp, #16]
	b 34f
35:
	cmp r1, #5
	bne 35f
	str r3, [sp, #20]
	b 34f
35:
	cmp r1, #6
	bne 35f
        str r3, [sp, #24]
        b 34f
35:
	cmp r1, #7
	bne 35f
	mov r7, r3
	b 34f
35:
	cmp r1, #8
	bne 35f
	mov r8, r3
	b 34f
35:
	cmp r1, #9
	bne 35f
	mov r9, r3
	b 34f
35:
	cmp r1, #10
	bne 35f
	mov r10, r3
	b 34f
35:
	cmp r1, #11
	bne 35f
	mov r11, r3
	b 34f
35:
	cmp r1, #12
	bne 35f
	mov r12, r3
	b 34f
35:
	cmp r1, #13
	bne 34f
	msr sp_svc, r3
34:
.endm

.macro fix_base_reg_str
	// Check if the STR updates the base register
	//// Get the fault instruction 
	mrs r0, ELR_hyp
	ldr r0, [r0]
	//// Decode and check the Write-back bit (W:21)
	lsr r1, r0, #21
	and r1, #1
	//// If W==1, then update the base reg.
	cmp r1, #1
	bne 34f
	//// Get the base reg number
	lsr r1, r0, #16
	and r1, #0xf
	//// Get the fault address that is trapped to the hyp
	cmp r1, #0
	bne 35f
	str r3, [sp]
	b 34f
35:
	cmp r1, #1
	bne 35f
	str r3, [sp, #4]
	b 34f
35:
	cmp r1, #2
	bne 35f
	str r3, [sp, #8]
	b 34f
35:
	cmp r1, #3
	bne 35f
	str r3, [sp, #12]
	b 34f
35:
	cmp r1, #4
	bne 35f
	str r3, [sp, #16]
	b 34f
35:
	cmp r1, #5
	bne 35f
	str r3, [sp, #20]
	b 34f
35:
	cmp r1, #6
	bne 35f
        str r3, [sp, #24]
        b 34f
35:
	cmp r1, #7
	bne 35f
	mov r7, r3
	b 34f
35:
	cmp r1, #8
	bne 35f
	mov r8, r3
	b 34f
35:
	cmp r1, #9
	bne 35f
	mov r9, r3
	b 34f
35:
	cmp r1, #10
	bne 35f
	mov r10, r3
	b 34f
35:
	cmp r1, #11
	bne 35f
	mov r11, r3
	b 34f
35:
	cmp r1, #12
	bne 35f
	mov r12, r3
	b 34f
35:
	cmp r1, #13
	bne 34f
	msr sp_svc, r3
34:
.endm

.macro copy_user_cb origVal
	ldr r4, =0x0
	cmp r5, r4
	beq 47f		// Kernel address
	
	and r0, r3, #0x0000000f	// check whether the request is CS (control and status => offset 0x0) or cblock(control block => offset 0x4)
	cmp r0, #0x4
	bne 47f
	
	ldr r0, =MYTEE_COPY_CONTROL_BLOCKS_IN_USER		// Call monitor to copy control blocks in user space to kernel space
	
	sub r4, \origVal, #MYTEE_PHYS_TO_VIRT_OFFSET		// Physical address of cb
		
	mov 	r2, #OFFSET_4KB
	mrc     p15, 0, lr, c0, c0, 5              @ get processor id:MPIDR
	and	lr, lr, #0xf
	mul	r2, r2, lr
	ldr	lr, =SECURE_BUFFER_BASE_VIRT             // Secure buffer base addr
	add	r2, r2, lr
	
	.arch_extension sec
        smc #2		// Monitor will copy control blocks in user space [r4] to the hypervisor space [r2]
	ldr	r0, =0x1	// Control blocks are already copied, set up the flag
	b	48f
47:
	ldr	r0, =0x0
48:
.endm


//TODO: Validate the read request
/**emul_read **/
/**r0:HSR     **/
/**r1: access size **/
/**r2: sign extension flag **/
/**r3: target address (read or write)**/
.macro emul_read, value
	cmp r2, #1	//check sign extension
	bne 2f
	cmp r1, #0	//check acess size. Byte
	bne 3f
	mcr p15, 0, r3, c7, c6, 1       //data cache inv.  mva poc
	ldrsb concatReg(\value), [r3]
	fix_base_reg_ldr \value
	dsb
	b 4f
3:	
	cmp r1, #1	// halfword
	bne 3f
	mcr p15, 0, r3, c7, c6, 1       //data cache inv.  mva poc
	ldrsh concatReg(\value), [r3]	
	dsb
	fix_base_reg_ldr \value
	b 4f
3:	
	cmp r1, #2	// word. Actually, no sign extension for ldr
	bne 3f
	mcr p15, 0, r3, c7, c6, 1       //data cache inv.  mva poc
	ldr concatReg(\value), [r3]
	fix_base_reg_ldr \value
	dsb
	b 4f
3:
	cmp r1, #3  	// We don't handle double word
	b 3b		// This will panic the system
	
// No sign extension
2:
	cmp r1, #0	//check acess size. Byte
	bne 3f
	mcr p15, 0, r3, c7, c6, 1       //data cache inv.  mva poc
	ldrb concatReg(\value), [r3]	
	dsb
	fix_base_reg_ldr \value
	b 4f
3:	
	cmp r1, #1	// halfword
	bne 3f
	mcr p15, 0, r3, c7, c6, 1       //data cache inv.  mva poc
	ldrh concatReg(\value), [r3]	
	dsb
	fix_base_reg_ldr \value
	b 4f
3:	
	cmp r1, #2	// word
	bne 3f
	mcr p15, 0, r3, c7, c6, 1       //data cache inv.  mva poc
	ldr concatReg(\value), [r3]
	dsb
	fix_base_reg_ldr \value
	b 4f
3:
	cmp r1, #3  	// We don't handle double word
	b 3b		// This will panic the system
	
//	.IF \value > 6 //From r7, we can simply restore all used registers
        //Restore all regs. 
4:       
        ldr r0, [sp]
        ldr r1, [sp,#4]
        ldr r2, [sp,#8]
        ldr r3, [sp,#12]
        ldr r4, [sp,#16]
        ldr r5, [sp,#20]
        ldr r6, [sp,#24]
        mrs lr, ELR_hyp
        add lr, #4      //nest inst after faulting inst.
        msr ELR_hyp, lr
        __ERET
//	.ENDIF
.endm

/**emul_write **/
/**r0:HSR     **/
/**r1: access size **/
/**r2: original value of r0~r6  **/
/**r3: target address (read or write)**/
.macro emul_write, value

//Before emulating the write, security verification needs to be performed
//DMA-filters are implemented here

//@ Filetering case 1: USB-DMA handling

//For USB-DMA packet fileter, MMIO virt address: 0xF09805XX
//hcdma: Offset: 500h + (chan_num * 20h) + 14h
//hcchar: Offset: 500h + (chan_num * 20h) + 00h
//LED request xfer_buff value: 0x2000921
.IF \value > 6
mov r2, concatReg(\value)
.ELSE
.ENDIF

lsr r4, r3, #8		
ldr lr, =MYTEE_HOST_CONTROLLER_REGS_VIRT	// Host controller reg.
cmp r4, lr
bne 199f			//Not USB-DMA
//Check the last 1 byte for checking if the current packet is hcdma or hcchar 
mov r4, r3
and r4, #0xff
mov lr, #0
33:
cmp r4, #0x20
blt 44f
sub r4, #0x20
add lr, #1
b 33b

44:
cmp r4, #0x0	//hcchar
beq 55f
cmp r4, #0x14   //hcdma
beq 66f
b   99f		//Not both hcchar and hcdma
	
66:		//hcdma handling logic start
.IF \value > 6
mov r4, concatReg(\value)	
.ELSE
mov r4, r2
.ENDIF
sub r4, #BCM2837_BUS_VIRT_OFFSET		//Bus to virt address
mcr p15, 0, r4, c7, c6, 1

ldr r4, [r4]			//Get the value in buff
ldr r5, =VALUE_REQUEST_LED
cmp r4, r5		// TODO: decode and check scroll lock (other led key toggle must be ignore)
bne 67f
ldr r5, =MYTEE_SECURE_KBD_FLAG_VIRT	
ldr r5, [r5]
cmp r5, #0x1	// check LED ON flag 
bne 209f	// LED ON flag is not setted, and LED is locked, so this generic LED ON request is deny.
		// TODO: LED toggle off
b 99f
.LTORG
67:			
ldr r5, =MYTEE_SECURE_KBD_FLAG_VIRT	//Normal kbd urb (not LED)
ldr r5, [r5]
cmp r5, #0x1
bne 99f		// secure keyboard off (generic mode)
			
ldr r5, =MYTEE_HC_NUM_SAVE_VIRT	// if yes->lr contains hc_num, so we compare it with previously saved one
ldr r5, [r5]		//Location for hc_num storage
cmp lr, r5
bne 99f		

ldr r5, =MYTEE_USBHID_INBUF_DMA_VIRT // check whether DMA Address is equal to kernel usbhid->inbuf_dma
219:
ldr r0, [r5]
cmp r2, r0
beq 218f		// It's for in-buf for HID device (maybe keyboard)
cmp r0, #0x0		
beq 99f		// not equal inbuf
add r5, r5, #0x8
b 219b
218:

.IF \value > 6			// Here we replace the normal buf with the secure buf
ldr concatReg(\value), =MYTEE_SECURE_USBHID_INBUF_DMA_BUS
.ELSE
ldr r2, =MYTEE_SECURE_USBHID_INBUF_DMA_BUS
.ENDIF

//ldr r2, =0xcf100000 // if same, switch the in_buff address to 0xcf100000(bus address)
b 99f

.LTORG
55:			//hcchar handling logic start
ldr r5, =MYTEE_SECURE_KBD_FLAG_VIRT	// check the TA flag for secure KBD.
ldr r5, [r5]
cmp r5, #0x1
bne 99f
lsl r0, r2, #3
lsr r0, r0, #3
lsr r0, r0, #22	// hcchar.devaddr [29:23] bit
ldr r5, =MYTEE_DEVNUM_SAVE_VIRT	// kbd_devnum
ldr r5, [r5]
cmp r5, r0		// if hcchar.devaddr == kbd_devnum
bne 99f		// not keyboard
ldr r5, =MYTEE_HC_CHAR_SAVE_VIRT
str r2, [r5]		// save hccar
ldr r5, =MYTEE_HC_NUM_SAVE_VIRT	// Location for hc_num storage
str lr, [r5]		// save hc_num
b 99f
.LTORG
199:
//@ Filtering case 2: external DMA handling
// 2-1. copies CBs to secure memory
// 2-2. update first CB address to the copied one
	add	sp, sp, #(4 * 14)
	str	r0, [sp, #MYTEE_SP_OFFSET_CONTEXT_X0]
	str	r1, [sp, #MYTEE_SP_OFFSET_CONTEXT_X1]
	str	r2, [sp, #MYTEE_SP_OFFSET_CONTEXT_X2]
	str	r3, [sp, #MYTEE_SP_OFFSET_CONTEXT_X3]
	str	r4, [sp, #MYTEE_SP_OFFSET_CONTEXT_X4]
	str	r5, [sp, #MYTEE_SP_OFFSET_CONTEXT_X5]
	str	r6, [sp, #MYTEE_SP_OFFSET_CONTEXT_X6]
	str	r7, [sp, #MYTEE_SP_OFFSET_CONTEXT_X7]
	str	r8, [sp, #MYTEE_SP_OFFSET_CONTEXT_X8]
	str	r9, [sp, #MYTEE_SP_OFFSET_CONTEXT_X9]
	str	r10, [sp, #MYTEE_SP_OFFSET_CONTEXT_X10]
	str	r11, [sp, #MYTEE_SP_OFFSET_CONTEXT_X11]
	str	r12, [sp, #MYTEE_SP_OFFSET_CONTEXT_X12]
	
	mov 	r0, #OFFSET_4KB
	mrc     p15, 0, lr, c0, c0, 5              @ get processor id:MPIDR
	and	lr, lr, #0xf
	mul	r0, r0, lr
	ldr	lr, =SECURE_BUFFER_BASE_VIRT             //secure buffer base addr
	add	r0, r0, lr
	
	str	r0, [sp, #MYTEE_SP_OFFSET_DMA_SECURE_BUFFER]
	
	ldr	r0, =DMA_VIRT_ADDR	// check DMA address
	ldr	r6, =0xFFFFF000
	and	r8, r3, r6
	cmp	r8, r0
	bne	214f			
	
	and	r0, r3, #0x0000000f	// check request cs(control and status => offset 0x0) or cblock(control block => offset 0x4)
	cmp	r0, #0x4
	bne	214f			//exit_verify

	// check control blocks are already copied in secure buffer
	ldr	r0, [sp, #MYTEE_SP_OFFSET_CONTEXT_X0]
	cmp	r0, #0x1
	beq	210f	// already copy done
	
	// start copy cb
	ldr	r0, [sp, #MYTEE_SP_OFFSET_DMA_SECURE_BUFFER]
	ldr	r3, =0x8	
	sub	r2, r2, #BCM2837_BUS_VIRT_OFFSET	// busAddr -> virtAddr
201:	// copy cb to secure buffer
	ldr	r1, [r2]
	str	r1, [r0]
	add	r2, r2, #0x4
	add	r0, r0, #0x4
	subs	r3, r3, #0x1
	bne	201b	//copy_cb\value

	sub	r2, r2, #0xc
	ldr	r2, [r2]
	cmp	r2, #0x0
	beq	210f
	ldr	r3, =0x8	
	sub	r2, r2, #BCM2837_BUS_VIRT_OFFSET
	bne	201b	//copy_cb\value

210:	// copy end
	ldr	r0, [sp, #MYTEE_SP_OFFSET_DMA_SECURE_BUFFER]
208:	// start verify control block	
	ldr	r1, [r0]	// dma_cb->info
	and	r2, r1, #0x2	// check TD mode bit
	cmp	r2, #0x0
	bne	202f	//goto td_mode_verify
	
203: 	// non_td_mode_verify
	ldr	r2, [r0, #0x4]	// dma_cb->src
	ldr	r3, [r0, #0x8]	// dma_cb->dst
	ldr	r4, [r0, #0xc]	// dma_cb->length
	ldr	r8, =0x2	// counter
	
	and	r5, r2, #0xff000000
	cmp	r5, #MYTEE_BCM2835_IO_DEVICE_ADDRESS_BUS	// bcm2835 I/O device bus address which is fixed address, not need verify
	beq	206f
	
	sub	r5, r2, #BCM2837_BUS_PHYS_OFFSET // src_busAddr => src_physAddr
	mov	r6, r5
	add	r6, r6, r4	     // src + length - 1
	sub	r6, r6, #0x1
	

204:	// non_td_mode verify_loop
	// src and dst verify			
	// 1. [src+length-1] < KERNEL_TEXT_PHYS_START
	// 2. KERNEL_TEXT_PHYS_END =< src ~ [src+length-1] < HYP_EL3_OPTEE_PHYS_START
	// 3. HYP_EL3_OPTEE_PHYS_END =< src
	cmp	r6, #KERNEL_TEXT_PHYS_START		
	blt	206f					// 1. [src+length- 1] < KERNEL_TEXT_PHYS_START

	cmp	r5, #HYP_EL3_OPTEE_PHYS_END
	bge	206f					// 3. HYP_EL3_OPTEE_PHYS_END =< src
	
	
	cmp	r5, #KERNEL_TEXT_PHYS_END
	blt	217f	//verify fail				// 2-1. KERNEL_TEXT_PHYS_END =< src

	cmp	r6, #HYP_EL3_OPTEE_PHYS_START
	bge	217f	//verify fail				// 2-2. [src+length-1] < HYP_EL3_OPTEE_PHYS_START


206:	// src verify success
	subs	r8, r8, #1
	beq	207f// dst verify succes
	
	// dst verify
	and	r5, r3, #0xff000000
	cmp	r5, #MYTEE_BCM2835_IO_DEVICE_ADDRESS_BUS	// bcm2835 I/O device bus address which is fixed address, not need verify
	beq	207f
	sub	r5, r3, #BCM2837_BUS_PHYS_OFFSET // dst_busAddr => src_physAddr
	mov	r6, r5
	add	r6, r6, r4	     // dst + length - 1
	sub	r6, r6, #0x1
	b	204b//verify_loop\value

207:	// dst verify success, goto loop entry with next block address
	ldr	r5, [r0, #0x14]	// next cb address
	cmp	r5, #0x0		// verify end
	beq	213f
	add	r0, r0, #0x20		// set next cb
	b	208b			// goto non_td mode verify


202:
	//td_mode_verify
	ldr	r2, [r0, #0x4]		// dma_cb->src
	ldr	r3, [r0, #0x8]		// dma_cb->dst
	ldr	r4, [r0, #0xc]		// dma_cb->length
	and	r9, r4, #0x3fffffff
	lsr	r9, r9, #0x10		// YLENGTH
	lsl	r10, r4, #0x10
	lsr	r10, r10, #0x10	// XLENGTH
	ldr	r11, [r0, #0x10]	// dma_cb->stride
	
	mul	r12, r9, r10		// XLENGTH*YLENGTH
	lsl	r8, r11, #0x10
	lsr	r8, r8, #0x10		// S_STRIDE
	sub	r9, r9, #0x1
	mul	r8, r8, r9		// (YLENGTH-1)*S_STRIDE
	
	sub	r5, r2, #BCM2837_BUS_PHYS_OFFSET 	// src_busAddr => src_physAddr
	mov	r6, r5		
	
	add	r6, r6, r8		// src + (XLENGTH*YLENGTH) + ((YLENGTH-1)*S_STRIDE) - 1
	add	r6, r6, r12
	sub	r6, r6, #0x1
	
	lsr	r8, r11, #0x10		// D_STRIDE
	mul	r11, r9, r8		// (YLENGTH-1)*D_STRIDE
	
	ldr	r8, =0x2		// counter
		
	and	r5, r2, #0xff000000
	cmp	r5, #MYTEE_BCM2835_IO_DEVICE_ADDRESS_BUS	// bcm2835 I/O device bus address which is fixed address, not need verify
	beq	211f
	
	sub	r5, r2, #BCM2837_BUS_PHYS_OFFSET 	// src_busAddr => src_physAddr	
	

205:	// td_mode verify_loop
	// src and dst verify	
	// 1. [src + (XLENGTH*YLENGTH) + (YLENGTH-1)*S_STRIDE - 1] < KERNEL_TEXT_PHYS_START
	// 2. KERNEL_TEXT_PHYS_END =< src ~ [src + (XLENGTH*YLENGTH) + (YLENGTH-1)*S(or D)_STRIDE - 1] < HYP_EL3_OPTEE_PHYS_START
	// 3. HYP_EL3_OPTEE_PHYS_END =< src
	cmp	r6, #KERNEL_TEXT_PHYS_START		
	blt	211f					// 1. [src + (XLENGTH*YLENGTH) + (YLENGTH-1)*S_STRIDE - 1] < KERNEL_TEXT_PHYS_START

	cmp	r5, #HYP_EL3_OPTEE_PHYS_END
	bge	211f					// 3. HYP_EL3_OPTEE_PHYS_END =< src
	
	
	cmp	r5, #KERNEL_TEXT_PHYS_END
	blt	217f	//verify fail				// 2-1. KERNEL_TEXT_PHYS_END =< src

	cmp	r6, #HYP_EL3_OPTEE_PHYS_START
	bge	217f	//verify fail				// 2-2. [src + (XLENGTH*YLENGTH) + (YLENGTH-1)*S_STRIDE-1] < HYP_EL3_OPTEE_PHYS_START

211:	// src verify success
	subs	r8, r8, #1
	beq	212f// dst verify succes
	
	//dst verify	
	and	r5, r3, #0xff000000
	cmp	r5, #MYTEE_BCM2835_IO_DEVICE_ADDRESS_BUS	// bcm2835 I/O device bus address which is fixed address, not need verify
	beq	212f
	sub	r5, r3, #BCM2837_BUS_PHYS_OFFSET // dst_busAddr => dst_physAddr
	mov	r6, r5
	
	
	add	r6, r6, r12	     // dst + (XLENGTH*YLENGTH) + ((YLENGTH-1)*D_STRIDE) - 1
	add	r6, r6, r11
	sub	r6, r6, #0x1
	b	205b//verify_loop\value

212:	// dst verify success
	// goto loop entry with next block address
	ldr	r5, [r0, #0x14]	// next cb address
	cmp	r5, #0x0		// verify end
	beq	213f
	add	r0, r0, #0x20		// set next_cb to r0
	b	208b		

213:	// change next block address to secure buffer cb (DMA CB verify success)
	ldr	r0, [sp, #MYTEE_SP_OFFSET_DMA_SECURE_BUFFER]
	ldr	r5, =MYTEE_CONTROL_BLOCKS_LOG_PAGE
	add	r5, r5, #0x8
	
	mov	r1, r0
	add	r1, r1, #0x20
	add	r0, r0, #0x14

216:	
	ldr	r2, [r0]
	cmp	r2, #0
	beq	215f
220:	
	mov	lr, r2
	and	lr, #0xf0000000
	cmp	lr, #MYTEE_USER_SPACE_ADDRESS_BUS		// bus address of user space
	bne	221f //change_next_block

221://change_next_block:
	add	r1, r1, #BCM2837_BUS_VIRT_OFFSET	// secure buffer bus address
	str	r1, [r0]
	sub	r1, r1, #BCM2837_BUS_VIRT_OFFSET
	add	r0, r0, #0x20
	add	r1, r1, #0x20	
	b	216b

215:
	// load secure buffer to r2	(DMA CB verify success)
	ldr	r2, [sp, #MYTEE_SP_OFFSET_DMA_SECURE_BUFFER]
	add	r2, r2, #BCM2837_BUS_VIRT_OFFSET	// change to secure buffer bus address from first 
			// DMA CB address bus address(which be mapped to bcm2835 dma phys address 0x3f007000)
	b	200f
214:	// load saved context to r2	(verify skip)
	ldr	r2, [sp, #MYTEE_SP_OFFSET_CONTEXT_X2]
	
200:	//exit_verify (success)
	ldr	r0, [sp, #MYTEE_SP_OFFSET_CONTEXT_X0]
	ldr	r1, [sp, #MYTEE_SP_OFFSET_CONTEXT_X1]
	ldr	r3, [sp, #MYTEE_SP_OFFSET_CONTEXT_X3]
	ldr	r4, [sp, #MYTEE_SP_OFFSET_CONTEXT_X4]
	ldr	r5, [sp, #MYTEE_SP_OFFSET_CONTEXT_X5]
	ldr	r6, [sp, #MYTEE_SP_OFFSET_CONTEXT_X6]
	ldr	r7, [sp, #MYTEE_SP_OFFSET_CONTEXT_X7]
	ldr	r8, [sp, #MYTEE_SP_OFFSET_CONTEXT_X8]
	ldr	r9, [sp, #MYTEE_SP_OFFSET_CONTEXT_X9]
	ldr	r10, [sp, #MYTEE_SP_OFFSET_CONTEXT_X10]
	ldr	r11, [sp, #MYTEE_SP_OFFSET_CONTEXT_X11]
	ldr	r12, [sp, #MYTEE_SP_OFFSET_CONTEXT_X12]
	sub     sp, sp, #(4 * 14)
99:

// Perform write emulation
//.IF \value > 4 
.IF \value > 6 
	cmp r1, #0	//check acess size. Byte
	bne 3f
	strb concatReg(\value), [r3]
	dmb
	mcr p15, 0, r3, c7, c6, 1	//data cache inv.  mva poc
	dsb
	fix_base_reg_str
	b 4f
3:	
	cmp r1, #1	// halfword
	bne 3f
	strh concatReg(\value), [r3]	
	dmb
	mcr p15, 0, r3, c7, c6, 1	//data cache inv.  mva poc
	dsb
	fix_base_reg_str	
	b 4f
3:	
	cmp r1, #2	// word. 
	bne 3f
	str concatReg(\value), [r3]
	dmb
	mcr p15, 0, r3, c7, c6, 1	//data cache inv.  mva poc
	dsb
	fix_base_reg_str
	b 4f
3:
	cmp r1, #3  	// We don't handle double word
	b 4f		// This will panic the system
.ELSE // We use r2 for store
	cmp r1, #0	//check acess size. Byte
	bne 3f
	strb r2, [r3]
	dmb
	mcr p15, 0, r3, c7, c6, 1	//data cache inv.  mva poc
	dsb
	fix_base_reg_str
	b 4f
3:	
	cmp r1, #1	// halfword
	bne 3f
	strh r2, [r3]
	dmb
	mcr p15, 0, r3, c7, c6, 1	//data cache inv.  mva poc
	dsb
	fix_base_reg_str
	b 4f
3:	
	cmp r1, #2	// word. 
	bne 3f
	str r2, [r3]
	dmb
	mcr p15, 0, r3, c7, c6, 1	//data cache inv.  mva poc
	dsb
	fix_base_reg_str
	b 4f
3:
	cmp r1, #3  	// We don't handle double word
	b 3b		//This is infinite loop
.ENDIF	

4:
	b 209f
217:
	nop	// verify fail, for breakpoint
8:
	ldr	r0, [sp, #MYTEE_SP_OFFSET_CONTEXT_X0]
	ldr	r1, [sp, #MYTEE_SP_OFFSET_CONTEXT_X1]
	ldr	r2, [sp, #MYTEE_SP_OFFSET_CONTEXT_X2]
	ldr	r3, [sp, #MYTEE_SP_OFFSET_CONTEXT_X3]
	ldr	r4, [sp, #MYTEE_SP_OFFSET_CONTEXT_X4]
	ldr	r5, [sp, #MYTEE_SP_OFFSET_CONTEXT_X5]
	ldr	r6, [sp, #MYTEE_SP_OFFSET_CONTEXT_X6]
	ldr	r7, [sp, #MYTEE_SP_OFFSET_CONTEXT_X7]
	ldr	r8, [sp, #MYTEE_SP_OFFSET_CONTEXT_X8]
	ldr	r9, [sp, #MYTEE_SP_OFFSET_CONTEXT_X9]
	ldr	r10, [sp, #MYTEE_SP_OFFSET_CONTEXT_X10]
	ldr	r11, [sp, #MYTEE_SP_OFFSET_CONTEXT_X11]
	ldr	r12, [sp, #MYTEE_SP_OFFSET_CONTEXT_X12]
	sub	sp, sp, #(4 * 14)

209:
	//restore general registers
        ldr r0, [sp]
        ldr r1, [sp,#4]
        ldr r2, [sp,#8]
        ldr r3, [sp,#12]
        ldr r4, [sp,#16]
        ldr r5, [sp,#20]
        ldr r6, [sp,#24]
        mrs lr, ELR_hyp
        add lr, #4      //next inst after faulting inst.
        msr ELR_hyp, lr
        __ERET
.endm

@TODO: temporal use of TTBR0 with no access at s2 paging
ENTRY(do_read)
	.arch_extension virt
	lsr r4, r0, #24
	and r4, #1		// Get ISV value (does HSR provide correct value?)
	cmp r4, #0
	beq 18f
	mov r4, r0
	lsr r4, r4, #16
	and r4, r4, #0xf	// Get SRT (register number)
	b 19f

18:				// Get the dst reg by decoding the faulting inst.
	//// Get the fault instruction (TODO: put this logic into write-emul as well)
        mrs r4, ELR_hyp
        ldr r4, [r4]
	lsr r4, r4, #12
	and r4, r4, #0xf
	
19:	
	
	cmp r4, #0	//Check if it's r0
	bne 1f
	emul_read 0
1:
	cmp r4, #1
	bne 1f
	emul_read 1
1:
	cmp r4, #2
	bne 1f
	emul_read 2 
1:
	cmp r4, #3
	bne 1f
	emul_read 3 
1:
	cmp r4, #4
	bne 1f
	emul_read 4 
1:
	cmp r4, #5
	bne 1f
	emul_read 5
1:
	cmp r4, #6
	bne 1f
	emul_read 6 
1:
	cmp r4, #7
	bne 1f
	emul_read 7
1:
	cmp r4, #8
	bne 1f
	emul_read 8
1:
	cmp r4, #9 
	bne 1f
	emul_read 9 
1:
	cmp r4, #10
	bne 1f
	emul_read 10
1:
	cmp r4, #11
	bne 1f
	emul_read 11
1:
	cmp r4, #12
	bne 1f
	emul_read 12
1:
	cmp r4, #13
	bne 1f
	emul_read 13
1 :
	__ERET
ENDPROC(do_read)

ENTRY(do_write)
	.arch_extension virt
        tst r3, #0x80000000 @ check if it's user space
        ldr r5, =0x0		// is kernel address
	bne 7f			@ kernel address

	// From here, handle the case that user mmaps to DMA mmio
	mcr p15, 0, r3, c7, c8, 3	@ats1cuw
	isb
	mrrc p15, 0, r2, r4, c7	@get PAR 64bit  @ virt-to-phys conversion
	ldr r4, =0xfffff000
	and r2, r4
	ldr r4, =MYTEE_DMA_MMIO_ADDRESS_PHYS    @DMA mmio address (phys addr) 
	cmp r2, r4
	bne 8f
	ldr r2, =DMA_VIRT_ADDR	@ DMA address (virt addr. in kernel view)
	ldr r4, =0xfff
	and r4, r3, r4
	add r3, r2, r4		@ update the write address from user virt to kernel virt
	ldr r5, =0x1		// is user address
	b 7f
.LTORG
8:	@ change the HTTBR to TTBR0. This should be done in monitor to remove critical instructions

/*	MRRC p15,0,r2,r4,c2   @ get TTBR0
	mov r4, #0
	mcrr    p15, 4, r2, r4, c2	@HTTBR update
	isb
	mcr     p15, 0, r3, c8, c6, 1   @TLBIMVA
@	dsb
@	mcr p15, 0, r3, c7, c6, 1       //data cache inv.  mva poc
	dsb     ish
	mov lr, #1		@ This is a flag indicating HTTBR change
*/
// Free of use: r2, r4
// Reserved: r0, r1, r3

7:
	lsr r4, r0, #24
        and r4, #1              // Get ISV value (does HSR provide correct value?)
        cmp r4, #0
        beq 18f

	mov r4, r0
	lsr r4, r4, #16	
	and r4, r4, #0xf	// Check the register involved in the write operation
	b 19f
18:
	//// Get the fault instruction 
        mrs r4, ELR_hyp
        ldr r4, [r4]
        lsr r4, r4, #12			// Get src and dst regs
        and r4, r4, #0xf
19:
	cmp r4, #0	// Check if it's r0
	bne 1f
	ldr r2, [sp]			// Restore the original value
	copy_user_cb r2
	emul_write 0
1:
	cmp r4, #1
	bne 1f
	ldr r2, [sp, #4]                    // Restore the original value  
	copy_user_cb r2
	emul_write 1
1:
	cmp r4, #2
	bne 1f
	ldr r2, [sp, #8]	              // Restore original value
	copy_user_cb r2
	emul_write 2 
1:
	cmp r4, #3
	bne 1f
	ldr r2, [sp, #12]                    // Restore original value
	copy_user_cb r2
	emul_write 3 
1:
	cmp r4, #4
	bne 1f
	ldr r2, [sp, #16]                    //restore original value
	copy_user_cb r2
	emul_write 4 
1:
	cmp r4, #5
	bne 1f
	ldr r2, [sp, #20]                    //restore original value
	copy_user_cb r2
	emul_write 5
1:
	cmp r4, #6
	bne 1f
	ldr r2, [sp, #24]                    //restore original value
	copy_user_cb r2
	emul_write 6 
1:
	cmp r4, #7
	bne 1f
	mov r2, r7
	copy_user_cb r2
	emul_write 7
1:
	cmp r4, #8
	bne 1f
        mov r2, r8
        copy_user_cb r2
	emul_write 8
1:
	cmp r4, #9 
	bne 1f
        mov r2, r9
        copy_user_cb r2
	emul_write 9 
1:
	cmp r4, #10
	bne 1f
        mov r2, r10
        copy_user_cb r2
	emul_write 10
1:
	cmp r4, #11
	bne 1f
        mov r2, r11
        copy_user_cb r2
	emul_write 11
1:
	cmp r4, #12
	bne 1f
        mov r2, r12
        copy_user_cb r2
	emul_write 12
1:
	cmp r4, #13
	bne 1f
        mov r2, r13
        copy_user_cb r2
	emul_write 13
1 :
	__ERET
ENDPROC(do_write)
#endif

/*
 * __hyp_set_vectors: Call this after boot to set the initial hypervisor
 * vectors as part of hypervisor installation.  On an SMP system, this should
 * be called on each CPU.
 *
 * r0 must be the physical address of the new vector table (which must lie in
 * the bottom 4GB of physical address space.
 *
 * r0 must be 32-byte aligned.
 *
 * Before calling this, you must check that the stub hypervisor is installed
 * everywhere, by waiting for any secondary CPUs to be brought up and then
 * checking that BOOT_CPU_MODE_HAVE_HYP(__boot_cpu_mode) is true.
 *
 * If not, there is a pre-existing hypervisor, some CPUs failed to boot, or
 * something else went wrong... in such cases, trying to install a new
 * hypervisor is unlikely to work as desired.
 *
 * When you call into your shiny new hypervisor, sp_hyp will contain junk,
 * so you will need to set that to something sensible at the new hypervisor's
 * initialisation entry point.
 */
ENTRY(__hyp_set_vectors)
	mov	r1, r0
	mov	r0, #HVC_SET_VECTORS
	__HVC(0)
	ret	lr
ENDPROC(__hyp_set_vectors)

ENTRY(__hyp_soft_restart)
	mov	r1, r0
	mov	r0, #HVC_SOFT_RESTART
	__HVC(0)
	ret	lr
ENDPROC(__hyp_soft_restart)

ENTRY(__hyp_reset_vectors)
	mov	r0, #HVC_RESET_VECTORS
	__HVC(0)
	ret	lr
ENDPROC(__hyp_reset_vectors)
