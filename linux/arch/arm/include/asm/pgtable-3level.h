/*
 * arch/arm/include/asm/pgtable-3level.h
 *
 * Copyright (C) 2011 ARM Ltd.
 * Author: Catalin Marinas <catalin.marinas@arm.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */
#ifdef CONFIG_MYTEE
#include <asm/mytee.h>
#endif

#ifndef _ASM_PGTABLE_3LEVEL_H
#define _ASM_PGTABLE_3LEVEL_H

/*
 * With LPAE, there are 3 levels of page tables. Each level has 512 entries of
 * 8 bytes each, occupying a 4K page. The first level table covers a range of
 * 512GB, each entry representing 1GB. Since we are limited to 4GB input
 * address range, only 4 entries in the PGD are used.
 *
 * There are enough spare bits in a page table entry for the kernel specific
 * state.
 */
#define PTRS_PER_PTE		512
#define PTRS_PER_PMD		512
#define PTRS_PER_PGD		4

#define PTE_HWTABLE_PTRS	(0)
#define PTE_HWTABLE_OFF		(0)
#define PTE_HWTABLE_SIZE	(PTRS_PER_PTE * sizeof(u64))

/*
 * PGDIR_SHIFT determines the size a top-level page table entry can map.
 */
#define PGDIR_SHIFT		30

/*
 * PMD_SHIFT determines the size a middle-level page table entry can map.
 */
#define PMD_SHIFT		21

#define PMD_SIZE		(1UL << PMD_SHIFT)
#define PMD_MASK		(~((1 << PMD_SHIFT) - 1))
#define PGDIR_SIZE		(1UL << PGDIR_SHIFT)
#define PGDIR_MASK		(~((1 << PGDIR_SHIFT) - 1))

/*
 * section address mask and size definitions.
 */
#define SECTION_SHIFT		21
#define SECTION_SIZE		(1UL << SECTION_SHIFT)
#define SECTION_MASK		(~((1 << SECTION_SHIFT) - 1))

#define USER_PTRS_PER_PGD	(PAGE_OFFSET / PGDIR_SIZE)

/*
 * Hugetlb definitions.
 */
#define HPAGE_SHIFT		PMD_SHIFT
#define HPAGE_SIZE		(_AC(1, UL) << HPAGE_SHIFT)
#define HPAGE_MASK		(~(HPAGE_SIZE - 1))
#define HUGETLB_PAGE_ORDER	(HPAGE_SHIFT - PAGE_SHIFT)

/*
 * "Linux" PTE definitions for LPAE.
 *
 * These bits overlap with the hardware bits but the naming is preserved for
 * consistency with the classic page table format.
 */
#define L_PTE_VALID		(_AT(pteval_t, 1) << 0)		/* Valid */
#define L_PTE_PRESENT		(_AT(pteval_t, 3) << 0)		/* Present */
#define L_PTE_USER		(_AT(pteval_t, 1) << 6)		/* AP[1] */
#define L_PTE_SHARED		(_AT(pteval_t, 3) << 8)		/* SH[1:0], inner shareable */
#define L_PTE_YOUNG		(_AT(pteval_t, 1) << 10)	/* AF */
#define L_PTE_XN		(_AT(pteval_t, 1) << 54)	/* XN */
#define L_PTE_DIRTY		(_AT(pteval_t, 1) << 55)
#define L_PTE_SPECIAL		(_AT(pteval_t, 1) << 56)
#define L_PTE_NONE		(_AT(pteval_t, 1) << 57)	/* PROT_NONE */
#define L_PTE_RDONLY		(_AT(pteval_t, 1) << 58)	/* READ ONLY */

#define L_PMD_SECT_VALID	(_AT(pmdval_t, 1) << 0)
#define L_PMD_SECT_DIRTY	(_AT(pmdval_t, 1) << 55)
#define L_PMD_SECT_NONE		(_AT(pmdval_t, 1) << 57)
#define L_PMD_SECT_RDONLY	(_AT(pteval_t, 1) << 58)

/*
 * To be used in assembly code with the upper page attributes.
 */
#define L_PTE_XN_HIGH		(1 << (54 - 32))
#define L_PTE_DIRTY_HIGH	(1 << (55 - 32))

/*
 * AttrIndx[2:0] encoding (mapping attributes defined in the MAIR* registers).
 */
#define L_PTE_MT_UNCACHED	(_AT(pteval_t, 0) << 2)	/* strongly ordered */
#define L_PTE_MT_BUFFERABLE	(_AT(pteval_t, 1) << 2)	/* normal non-cacheable */
#define L_PTE_MT_WRITETHROUGH	(_AT(pteval_t, 2) << 2)	/* normal inner write-through */
#define L_PTE_MT_WRITEBACK	(_AT(pteval_t, 3) << 2)	/* normal inner write-back */
#define L_PTE_MT_WRITEALLOC	(_AT(pteval_t, 7) << 2)	/* normal inner write-alloc */
#define L_PTE_MT_DEV_SHARED	(_AT(pteval_t, 4) << 2)	/* device */
#define L_PTE_MT_DEV_NONSHARED	(_AT(pteval_t, 4) << 2)	/* device */
#define L_PTE_MT_DEV_WC		(_AT(pteval_t, 1) << 2)	/* normal non-cacheable */
#define L_PTE_MT_DEV_CACHED	(_AT(pteval_t, 3) << 2)	/* normal inner write-back */
#define L_PTE_MT_MASK		(_AT(pteval_t, 7) << 2)

/*
 * Software PGD flags.
 */
#define L_PGD_SWAPPER		(_AT(pgdval_t, 1) << 55)	/* swapper_pg_dir entry */

/*
 * 2nd stage PTE definitions for LPAE.
 */
#define L_PTE_S2_MT_UNCACHED		(_AT(pteval_t, 0x0) << 2) /* strongly ordered */
#define L_PTE_S2_MT_WRITETHROUGH	(_AT(pteval_t, 0xa) << 2) /* normal inner write-through */
#define L_PTE_S2_MT_WRITEBACK		(_AT(pteval_t, 0xf) << 2) /* normal inner write-back */
#define L_PTE_S2_MT_DEV_SHARED		(_AT(pteval_t, 0x1) << 2) /* device */
#define L_PTE_S2_MT_MASK		(_AT(pteval_t, 0xf) << 2)

#define L_PTE_S2_RDONLY			(_AT(pteval_t, 1) << 6)   /* HAP[1]   */
#define L_PTE_S2_RDWR			(_AT(pteval_t, 3) << 6)   /* HAP[2:1] */

#define L_PMD_S2_RDONLY			(_AT(pmdval_t, 1) << 6)   /* HAP[1]   */
#define L_PMD_S2_RDWR			(_AT(pmdval_t, 3) << 6)   /* HAP[2:1] */

/*
 * Hyp-mode PL2 PTE definitions for LPAE.
 */
#define L_PTE_HYP		L_PTE_USER

#ifndef __ASSEMBLY__

#define pud_none(pud)		(!pud_val(pud))
#define pud_bad(pud)		(!(pud_val(pud) & 2))
#define pud_present(pud)	(pud_val(pud))
#define pmd_table(pmd)		((pmd_val(pmd) & PMD_TYPE_MASK) == \
						 PMD_TYPE_TABLE)
#define pmd_sect(pmd)		((pmd_val(pmd) & PMD_TYPE_MASK) == \
						 PMD_TYPE_SECT)
#define pmd_large(pmd)		pmd_sect(pmd)

#ifdef CONFIG_MYTEE
#define EMUL_PUD_CLEAR 13
static inline void pud_clear(pud_t *pudp)
{
        unsigned long cmd_id =  EMUL_PUD_CLEAR;
        unsigned long pudp_addr = (unsigned long)pudp;
	if (is_mytee_ro_page((unsigned long)pudp)) {
        	__asm__ __volatile__ (".arch_extension sec");
	        cpu_dcache_clean_area(pudp, 8);
	        __asm__ __volatile__ (
       		 "stmfd  sp!,{r0-r2}\n"
	        "mov    r0, %0\n"
	        "mov    r1, %1\n"
	        "mov    r2, r1\n"
	        "mcr    p15, 0, r1, c7, c14, 1\n"
	        "add    r1, r1, #4\n"
	        "mcr    p15, 0, r1, c7, c14, 1\n"
	        "mov    r1, r2\n"
	        "dsb\n"
	        "smc    #0\n"
	        "mcr    p15, 0, r2, c7, c6,  1\n"
	        "dsb\n"
	        "add    r2, r2, #4\n"
	        "mcr    p15, 0, r2, c7, c6,  1\n"
	        "dsb\n"
	        "mov    r0, #0\n"
	        "mcr    p15, 0, r0, c8, c3, 0\n"
	        "dsb\n"
	        "isb\n"
	        "pop    {r0-r2}\n"
	        ::"r"(cmd_id),"r"(pudp_addr):"r0","r1","cc");
	}
	else{
		*pudp = __pud(0);
	}
        clean_pmd_entry(pudp);
}

#else
#define pud_clear(pudp)			\
	do {				\
		*pudp = __pud(0);	\
		clean_pmd_entry(pudp);	\
	} while (0)
#endif

#ifdef CONFIG_MYTEE
#define EMUL_SET_PUD 12

static inline void set_pud(pud_t *pudp, pud_t pud)
{
	unsigned long cmd_id = EMUL_SET_PUD;
	unsigned long pud_low, pud_high;
	unsigned long pudp_addr = (u32)pudp;

	pud_low = (unsigned long)(0xffffffff & pud_val(pud));
	pud_high = (unsigned long)(pud_val(pud) >> 32);

        if (is_mytee_ro_page((u32)pudp)) {
                
		__asm__ __volatile__(".arch_extension sec");
        	cpu_dcache_clean_area(pudp, 8);
	        __asm__ __volatile__ (
	        "stmfd  sp!,{r0-r4}\n"
	        "mov    r0, %0\n"
	        "mov    r1, %1\n"
	        "mov    r2, %2\n"
	        "mov    r3, %3\n"
	        "mcr    p15, 0, r1, c7, c14, 1\n"
	        "add    r1, r1, #4\n"
	        "mcr    p15, 0, r1, c7, c14, 1\n"
	        "sub    r1, r1, #4\n"
	        "mov    r4, r1\n"
	        "dsb\n"
	        "smc    #0\n"
	        "mcr    p15, 0, r4, c7, c6,  1\n"	
	        "dsb\n"
	        "add    r4, r4, #4\n"
	        "mcr    p15, 0, r4, c7, c6,  1\n"
	        "dsb\n"

	        "mov    r0, #0\n"
	        "mcr    p15, 0, r0, c8, c3, 0\n"
	        "dsb\n"
	        "isb\n"
	        "pop    {r0-r4}\n"
        	::"r"(cmd_id),"r"(pudp_addr),"r"(pud_low),"r"(pud_high):"r0","r1","r2","r3","r4","cc");
	
        } else {
		*pudp = pud;        
	}
	flush_pmd_entry(pudp);
}
#else
#define set_pud(pudp, pud)		\
	do {				\
		*pudp = pud;		\
		flush_pmd_entry(pudp);	\
	} while (0)
#endif

static inline pmd_t *pud_page_vaddr(pud_t pud)
{
	return __va(pud_val(pud) & PHYS_MASK & (s32)PAGE_MASK);
}

/* Find an entry in the second-level page table.. */
#define pmd_index(addr)		(((addr) >> PMD_SHIFT) & (PTRS_PER_PMD - 1))
static inline pmd_t *pmd_offset(pud_t *pud, unsigned long addr)
{
	return (pmd_t *)pud_page_vaddr(*pud) + pmd_index(addr);
}

#define pmd_bad(pmd)		(!(pmd_val(pmd) & 2))

#ifdef CONFIG_MYTEE

#define EMUL_COPY_PMD	9
static inline void copy_pmd(pmd_t *pmdpd, pmd_t *pmdps)
{
	unsigned long cmd_id = EMUL_COPY_PMD;
	unsigned long pmdpd_addr, pmdps_addr;
	pmdpd_addr = (unsigned long) pmdpd;
	pmdps_addr = (unsigned long) pmdps;
	
	__asm__ __volatile__ (".arch_extension sec"); 
	cpu_dcache_clean_area(pmdpd, 8);
	__asm__ __volatile__ (
    	"stmfd  sp!,{r0-r4}\n"
    	"mov	r0, %0\n"
	"mov	r1, %1\n"
    	"mov	r2, %2\n"
	"mov    r4, r1\n"
    	"mcr	p15, 0, r1, c7, c14, 1\n"
    	"add	r1, r1, #4\n"
    	"mcr	p15, 0, r1, c7, c14, 1\n"
	"sub    r1, r1, #4\n"
    	"dsb\n"
    	"smc	#0\n"
    	"mcr	p15, 0, r4, c7, c6,  1\n"
    	"dsb\n"
    	"add	r4, r4, #4\n"
    	"mcr	p15, 0, r4, c7, c6,  1\n"
    	"dsb\n"
    	"mov	r0, #0\n"
    	"mcr	p15, 0, r0, c8, c3, 0\n"
    	"dsb\n"
    	"isb\n"
    	"pop	{r0-r4}\n"
    	::"r"(cmd_id),"r"(pmdpd_addr),"r"(pmdps_addr):"r0","r1","r2","cc");
 
    	flush_pmd_entry(pmdpd);
}

#else
#define copy_pmd(pmdpd,pmdps)		\
	do {				\
		*pmdpd = *pmdps;	\
		flush_pmd_entry(pmdpd);	\
	} while (0)

#endif // CONFIG_MYTEE

#ifdef CONFIG_MYTEE

#define EMUL_PMD_CLEAR 10
static inline void pmd_clear(pmd_t *pmdp)
{
	unsigned long cmd_id =  EMUL_PMD_CLEAR;
	unsigned long pmdp_addr = (unsigned long)pmdp;
 
	__asm__ __volatile__ (".arch_extension sec");
	cpu_dcache_clean_area(pmdp, 8);
	__asm__ __volatile__ (
    	"stmfd  sp!,{r0-r2}\n"
    	"mov	r0, %0\n"
    	"mov	r1, %1\n"
    	"mov	r2, r1\n"
    	"mcr	p15, 0, r1, c7, c14, 1\n"
        "add	r1, r1, #4\n"
    	"mcr	p15, 0, r1, c7, c14, 1\n"
	"mov    r1, r2\n"
    	"dsb\n"
    	"smc	#0\n"
    	"mcr	p15, 0, r2, c7, c6,  1\n"
    	"dsb\n"
    	"add	r2, r2, #4\n"
    	"mcr	p15, 0, r2, c7, c6,  1\n"
    	"dsb\n"
    	"mov	r0, #0\n"
    	"mcr	p15, 0, r0, c8, c3, 0\n"
    	"dsb\n"
    	"isb\n"
    	"pop	{r0-r2}\n"
    	::"r"(cmd_id),"r"(pmdp_addr):"r0","r1","cc");
 
    	clean_pmd_entry(pmdp);
}

#else
#define pmd_clear(pmdp)			\
	do {				\
		*pmdp = __pmd(0);	\
		clean_pmd_entry(pmdp);	\
	} while (0)
#endif	//CONFIG_MYTEE
/*
 * For 3 levels of paging the PTE_EXT_NG bit will be set for user address ptes
 * that are written to a page table but not for ptes created with mk_pte.
 *
 * In hugetlb_no_page, a new huge pte (new_pte) is generated and passed to
 * hugetlb_cow, where it is compared with an entry in a page table.
 * This comparison test fails erroneously leading ultimately to a memory leak.
 *
 * To correct this behaviour, we mask off PTE_EXT_NG for any pte that is
 * present before running the comparison.
 */
#define __HAVE_ARCH_PTE_SAME
#define pte_same(pte_a,pte_b)	((pte_present(pte_a) ? pte_val(pte_a) & ~PTE_EXT_NG	\
					: pte_val(pte_a))				\
				== (pte_present(pte_b) ? pte_val(pte_b) & ~PTE_EXT_NG	\
					: pte_val(pte_b)))

#define set_pte_ext(ptep,pte,ext) cpu_set_pte_ext(ptep,__pte(pte_val(pte)|(ext)))

#define pte_huge(pte)		(pte_val(pte) && !(pte_val(pte) & PTE_TABLE_BIT))
#define pte_mkhuge(pte)		(__pte(pte_val(pte) & ~PTE_TABLE_BIT))

#define pmd_isset(pmd, val)	((u32)(val) == (val) ? pmd_val(pmd) & (val) \
						: !!(pmd_val(pmd) & (val)))
#define pmd_isclear(pmd, val)	(!(pmd_val(pmd) & (val)))

#define pmd_present(pmd)	(pmd_isset((pmd), L_PMD_SECT_VALID))
#define pmd_young(pmd)		(pmd_isset((pmd), PMD_SECT_AF))
#define pte_special(pte)	(pte_isset((pte), L_PTE_SPECIAL))
static inline pte_t pte_mkspecial(pte_t pte)
{
	pte_val(pte) |= L_PTE_SPECIAL;
	return pte;
}
#define	__HAVE_ARCH_PTE_SPECIAL

#define __HAVE_ARCH_PMD_WRITE
#define pmd_write(pmd)		(pmd_isclear((pmd), L_PMD_SECT_RDONLY))
#define pmd_dirty(pmd)		(pmd_isset((pmd), L_PMD_SECT_DIRTY))
#define pud_page(pud)		pmd_page(__pmd(pud_val(pud)))
#define pud_write(pud)		pmd_write(__pmd(pud_val(pud)))

#define pmd_hugewillfault(pmd)	(!pmd_young(pmd) || !pmd_write(pmd))
#define pmd_thp_or_huge(pmd)	(pmd_huge(pmd) || pmd_trans_huge(pmd))

#ifdef CONFIG_TRANSPARENT_HUGEPAGE
#define pmd_trans_huge(pmd)	(pmd_val(pmd) && !pmd_table(pmd))
#endif

#define PMD_BIT_FUNC(fn,op) \
static inline pmd_t pmd_##fn(pmd_t pmd) { pmd_val(pmd) op; return pmd; }

PMD_BIT_FUNC(wrprotect,	|= L_PMD_SECT_RDONLY);
PMD_BIT_FUNC(mkold,	&= ~PMD_SECT_AF);
PMD_BIT_FUNC(mkwrite,   &= ~L_PMD_SECT_RDONLY);
PMD_BIT_FUNC(mkdirty,   |= L_PMD_SECT_DIRTY);
PMD_BIT_FUNC(mkclean,   &= ~L_PMD_SECT_DIRTY);
PMD_BIT_FUNC(mkyoung,   |= PMD_SECT_AF);

#define pmd_mkhuge(pmd)		(__pmd(pmd_val(pmd) & ~PMD_TABLE_BIT))

#define pmd_pfn(pmd)		(((pmd_val(pmd) & PMD_MASK) & PHYS_MASK) >> PAGE_SHIFT)
#define pfn_pmd(pfn,prot)	(__pmd(((phys_addr_t)(pfn) << PAGE_SHIFT) | pgprot_val(prot)))
#define mk_pmd(page,prot)	pfn_pmd(page_to_pfn(page),prot)


#ifdef CONFIG_MYTEE
extern u8 mytee_init_flag;
int mytee_tee_mem_emul(u32, u32, u32, u32);
#define EMUL_MEM_PMD 8
#endif

/* represent a notpresent pmd by faulting entry, this is used by pmdp_invalidate */
static inline pmd_t pmd_mknotpresent(pmd_t pmd)
{
	return __pmd(pmd_val(pmd) & ~L_PMD_SECT_VALID);
}

static inline pmd_t pmd_modify(pmd_t pmd, pgprot_t newprot)
{
	const pmdval_t mask = PMD_SECT_USER | PMD_SECT_XN | L_PMD_SECT_RDONLY |
				L_PMD_SECT_VALID | L_PMD_SECT_NONE;
	pmd_val(pmd) = (pmd_val(pmd) & ~mask) | (pgprot_val(newprot) & mask);
	return pmd;
}

static inline void set_pmd_at(struct mm_struct *mm, unsigned long addr,
			      pmd_t *pmdp, pmd_t pmd)
{
#ifdef CONFIG_MYTEE
	unsigned long val_h, val_l;
#endif
	BUG_ON(addr >= TASK_SIZE);

	/* create a faulting entry if PROT_NONE protected */
	if (pmd_val(pmd) & L_PMD_SECT_NONE)
		pmd_val(pmd) &= ~L_PMD_SECT_VALID;

	if (pmd_write(pmd) && pmd_dirty(pmd))
		pmd_val(pmd) &= ~PMD_SECT_AP2;
	else
		pmd_val(pmd) |= PMD_SECT_AP2;
#ifdef CONFIG_MYTEE
//	if(mytee_init_flag){
		val_h = (u32)(__pmd(pmd_val(pmd) | PMD_SECT_nG) >> 32);
		val_l = (u32)(__pmd(pmd_val(pmd) | PMD_SECT_nG) & 0xffffffff);
		mytee_tee_mem_emul(EMUL_MEM_PMD, (u32)pmdp, val_l, val_h);
//	}
//	else
//		*pmdp = __pmd(pmd_val(pmd) | PMD_SECT_nG);
#else
	*pmdp = __pmd(pmd_val(pmd) | PMD_SECT_nG);
#endif
	flush_pmd_entry(pmdp);
}

#endif /* __ASSEMBLY__ */

#endif /* _ASM_PGTABLE_3LEVEL_H */
