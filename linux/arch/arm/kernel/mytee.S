#ifdef CONFIG_MYTEE
#include <linux/linkage.h>
#include <asm/assembler.h>
#define KERNEL_TEXT_PHYS_START 0x00200000
#define KERNEL_TEXT_PHYS_END 0x00a00000

#define __SMC(imm4) __inst_arm_thumb32(                                 \
        0xE1600070 | (((imm4) & 0xF) << 0),                             \
        0xF7F08000 | (((imm4) & 0xF) << 16)                             \
)


ENTRY(mytee_lazy_cpu1_setup)
	__HVC(0)
	ret lr
ENDPROC(mytee_lazy_cpu1_setup)

ENTRY(mytee_enable_hyp_mmu)
	__HVC(0)
	ret lr
ENDPROC(mytee_enable_hyp_mmu)

/*
ENTRY(mytee_kernel_text_ro)
	__HVC(0)
	ret lr
ENDPROC(mytee_kernel_text_ro)
*/

ENTRY(mytee_kernel_text_ro)
	mov r0, #98
        __SMC(2)
        ret lr
ENDPROC(mytee_kernel_text_ro)


ENTRY(mytee_set_httbr_ttbr1)
	__HVC(0)
	ret lr
ENDPROC(mytee_set_httbr_ttbr1)

ENTRY(mytee_set_httbr_ttbr0)
	__HVC(0)
	ret lr
ENDPROC(mytee_set_httbr_ttbr0)

ENTRY(mytee_memcopy)
	__HVC(0)
	ret lr
ENDPROC(mytee_memcopy)

ENTRY(mytee_up_priv)
//	mov r1, lr	
	mov	r0, #99
//	mov	r2, lr
	__HVC(0)
ENDPROC(mytee_up_priv)

/*
ENTRY(mytee_down_priv)
	teq	r1, #0
	bne	1f
	mrs 	r0, spsr	// M[4:0] might contain hyp mode
	bic 	r0, r0, #0x1f	// clear M[4:0]
	orr 	r0, r0, #0x13	// Change to svc mode
	msr	spsr_cxsf, r0	//update spsr
	//ELR_hyp may contain a right return address
	// msr	ELR_hyp, r14
	mov	r1, r14
	__ERET
1:
	bx	r1

ENDPROC(mytee_down_priv)
*/
ENTRY(mytee_down_priv)
	.arch_extension virt
//	teq	r1, #0
//	bne	1f
//	mrs 	r0, spsr	// M[4:0] might contain hyp mode
//	bic 	r0, r0, #0x1f	// clear M[4:0]
//	orr 	r0, r0, #0x13	// Change to svc mode
//	msr	spsr_cxsf, r0	//update spsr
	//ELR_hyp may contain right address to return
        msr	ELR_hyp, r14
//	mov	r1, r14
	__ERET
//1:
//	bx	r1

ENDPROC(mytee_down_priv)

ENTRY(mytee_usbhid_inbuf_dma_save)
	__HVC(0)
	ret lr
ENDPROC(mytee_usbhid_inbuf_dma_save)

ENTRY(mytee_unmmap_fb)
	.arch_extension sec
        smc #2
        ret lr
ENDPROC(mytee_unmmap_fb)

ENTRY(mytee_remmap_fb)
	.arch_extension sec
        smc #2
        ret lr
ENDPROC(mytee_remmap_fb)

ENTRY(mytee_shield_mmio)
        .arch_extension sec
        smc #2
        ret lr
ENDPROC(mytee_shield_mmio)

ENTRY(mytee_unshield_mmio)
        .arch_extension sec
        smc #2
        ret lr
ENDPROC(mytee_unshield_mmio)

ENTRY(mytee_shield_mmio_4kb)
        .arch_extension sec
        smc #2
        ret lr
ENDPROC(mytee_shield_mmio_4kb)

ENTRY(mytee_unshield_mmio_4kb)
        .arch_extension sec
        smc #2
        ret lr
ENDPROC(mytee_unshield_mmio_4kb)

ENTRY(mytee_dma_set_non_cacheable)
	.arch_extension sec
        smc #2
        ret lr		
ENDPROC(mytee_dma_set_non_cacheable)

ENTRY(mytee_new_pgd)
        .arch_extension sec
        smc #2
        ret lr
ENDPROC(mytee_new_pgd)

ENTRY(mytee_pgd_free)
        .arch_extension sec
        smc #2
        ret lr
ENDPROC(mytee_pgd_free)

ENTRY(mytee_ro_memset)
        .arch_extension sec
        smc #2
        ret lr
ENDPROC(mytee_ro_memset)

ENTRY(mytee_ro_memcpy)
        .arch_extension sec
        smc #2
        ret lr
ENDPROC(mytee_ro_memcpy)

ENTRY(mytee_init_monitor)
        .arch_extension sec
        smc #2
        ret lr
ENDPROC(mytee_init_monitor)

ENTRY(mytee_shield_mmio_with_phys)
        .arch_extension sec
        smc #2
        ret lr
ENDPROC(mytee_shield_mmio_with_phys)

ENTRY(mytee_unshield_mmio_with_phys)
        .arch_extension sec
        smc #2
        ret lr
ENDPROC(mytee_unshield_mmio_with_phys)

ENTRY(mytee_get_kernel_phys)
        .arch_extension sec
        smc #2
        ret lr
ENDPROC(mytee_get_kernel_phys)

#endif
