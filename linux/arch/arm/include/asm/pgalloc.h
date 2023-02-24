/*
 *  arch/arm/include/asm/pgalloc.h
 *
 *  Copyright (C) 2000-2001 Russell King
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef _ASMARM_PGALLOC_H
#define _ASMARM_PGALLOC_H

#include <linux/pagemap.h>

#include <asm/domain.h>
#include <asm/pgtable-hwdef.h>
#include <asm/processor.h>
#include <asm/cacheflush.h>
#include <asm/tlbflush.h>

#ifdef CONFIG_MYTEE
#include <asm/mytee.h>
#endif

#define check_pgt_cache()		do { } while (0)

#ifdef CONFIG_MMU

#define _PAGE_USER_TABLE	(PMD_TYPE_TABLE | PMD_BIT4 | PMD_DOMAIN(DOMAIN_USER))
#define _PAGE_KERNEL_TABLE	(PMD_TYPE_TABLE | PMD_BIT4 | PMD_DOMAIN(DOMAIN_KERNEL))

#ifdef CONFIG_ARM_LPAE

#ifdef CONFIG_MYTEE
static inline pmd_t *pmd_alloc_one(struct mm_struct *mm, unsigned long addr)
{
	pmd_t *ropage = NULL;

        ropage = (pmd_t *)mytee_ro_alloc();
        if (ropage){
//		memset(ropage, 0, 512);  
		mytee_ro_memset(EMUL_RO_MEMSET, ropage, 0, 0x1000, 0, 0);
		
                return ropage;
	}

	return (pmd_t *)get_zeroed_page(GFP_KERNEL);
}

#else
static inline pmd_t *pmd_alloc_one(struct mm_struct *mm, unsigned long addr)
{
	return (pmd_t *)get_zeroed_page(GFP_KERNEL);
}
#endif

#ifdef CONFIG_MYTEE
static inline void pmd_free(struct mm_struct *mm, pmd_t *pmd)
{
	BUG_ON((unsigned long)pmd & (PAGE_SIZE-1));
	if(is_mytee_ro_page((unsigned long)pmd))
		mytee_ro_free((void*)pmd);
	else
	        free_page((unsigned long)pmd);
}
#else
static inline void pmd_free(struct mm_struct *mm, pmd_t *pmd)
{
	BUG_ON((unsigned long)pmd & (PAGE_SIZE-1));
	free_page((unsigned long)pmd);
}
#endif

static inline void pud_populate(struct mm_struct *mm, pud_t *pud, pmd_t *pmd)
{
	set_pud(pud, __pud(__pa(pmd) | PMD_TYPE_TABLE));
}

#else	/* !CONFIG_ARM_LPAE */

/*
 * Since we have only two-level page tables, these are trivial
 */
#define pmd_alloc_one(mm,addr)		({ BUG(); ((pmd_t *)2); })
#define pmd_free(mm, pmd)		do { } while (0)
#define pud_populate(mm,pmd,pte)	BUG()

#endif	/* CONFIG_ARM_LPAE */

extern pgd_t *pgd_alloc(struct mm_struct *mm);
extern void pgd_free(struct mm_struct *mm, pgd_t *pgd);

#define PGALLOC_GFP	(GFP_KERNEL | __GFP_ZERO)

static inline void clean_pte_table(pte_t *pte)
{
	clean_dcache_area(pte + PTE_HWTABLE_PTRS, PTE_HWTABLE_SIZE);
}

/*
 * Allocate one PTE table.
 *
 * This actually allocates two hardware PTE tables, but we wrap this up
 * into one table thus:
 *
 *  +------------+
 *  | Linux pt 0 |
 *  +------------+
 *  | Linux pt 1 |
 *  +------------+
 *  |  h/w pt 0  |
 *  +------------+
 *  |  h/w pt 1  |
 *  +------------+
 */
static inline pte_t *
pte_alloc_one_kernel(struct mm_struct *mm, unsigned long addr)
{
	pte_t *pte;
	pte = (pte_t *)__get_free_page(PGALLOC_GFP);
	if (pte)
		clean_pte_table(pte);
	return pte;
}

static inline pgtable_t
pte_alloc_one(struct mm_struct *mm, unsigned long addr)
{
	struct page *pte;

#ifdef CONFIG_HIGHPTE
	pte = alloc_pages(PGALLOC_GFP | __GFP_HIGHMEM, 0);
#else
	pte = alloc_pages(PGALLOC_GFP, 0);
#endif
	if (!pte)
		return NULL;
	if (!PageHighMem(pte))
		clean_pte_table(page_address(pte));
	if (!pgtable_page_ctor(pte)) {
		__free_page(pte);
		return NULL;
	}
	return pte;
}

/*
 * Free one PTE table.
 */
static inline void pte_free_kernel(struct mm_struct *mm, pte_t *pte)
{
	if (pte)
		free_page((unsigned long)pte);
}

static inline void pte_free(struct mm_struct *mm, pgtable_t pte)
{
	pgtable_page_dtor(pte);
	__free_page(pte);
}
#ifdef CONFIG_MYTEE
#define EMUL_MEM_PMD 8 
#endif
static inline void __pmd_populate(pmd_t *pmdp, phys_addr_t pte,
				  pmdval_t prot)
{
	pmdval_t pmdval = (pte + PTE_HWTABLE_OFF) | prot;

#ifdef CONFIG_MYTEE
	unsigned long cmd_id = EMUL_MEM_PMD;
	unsigned long pmdval_low, pmdval_high;
	unsigned long pmdp_addr = (u32)pmdp;	//Assume 4GB only for kernel
	pmdval_low = (unsigned long)(0xffffffff & (u64)pmdval);
	pmdval_high = (unsigned long)(pmdval >> 32);
    	
	__asm__ __volatile__(".arch_extension sec");
	clean_dcache_area(pmdp, 8);
	__asm__ __volatile__ (
    	"stmfd  sp!,{r0-r4}\n"
    	"mov	r0, %0\n"
    	"mov	r1, %1\n"
    	"mov	r2, %2\n"
    	"mov	r3, %3\n"
    	"mcr	p15, 0, r1, c7, c14, 1\n"
    	"add	r1, r1, #4\n"
    	"mcr	p15, 0, r1, c7, c14, 1\n"
	"sub    r1, r1, #4\n"
	"mov 	r4, r1\n"
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
    	::"r"(cmd_id),"r"(pmdp_addr),"r"(pmdval_low),"r"(pmdval_high):"r0","r1","r2","r3","r4","cc");

#else
	pmdp[0] = __pmd(pmdval);
#ifndef CONFIG_ARM_LPAE
	pmdp[1] = __pmd(pmdval + 256 * sizeof(pte_t));
#endif
#endif
	flush_pmd_entry(pmdp);
}

/*
 * Populate the pmdp entry with a pointer to the pte.  This pmd is part
 * of the mm address space.
 *
 * Ensure that we always set both PMD entries.
 */
static inline void
pmd_populate_kernel(struct mm_struct *mm, pmd_t *pmdp, pte_t *ptep)
{
	/*
	 * The pmd must be loaded with the physical address of the PTE table
	 */
	__pmd_populate(pmdp, __pa(ptep), _PAGE_KERNEL_TABLE);
}

static inline void
pmd_populate(struct mm_struct *mm, pmd_t *pmdp, pgtable_t ptep)
{
	extern pmdval_t user_pmd_table;
	pmdval_t prot;

	if (__LINUX_ARM_ARCH__ >= 6 && !IS_ENABLED(CONFIG_ARM_LPAE))
		prot = user_pmd_table;
	else
		prot = _PAGE_USER_TABLE;

	__pmd_populate(pmdp, page_to_phys(ptep), prot);
}
#define pmd_pgtable(pmd) pmd_page(pmd)

#endif /* CONFIG_MMU */

#endif
