/*
 * Copyright (c) 2015-2018, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <arch_helpers.h>
#include <assert.h>
#include <bl_common.h>
#include <console.h>
#include <debug.h>
#include <interrupt_mgmt.h>
#include <platform_def.h>
#include <uart_16550.h>
#include <xlat_tables_v2.h>
#include "rpi3_hw.h"
#include "rpi3_private.h"

#define CONFIG_MYTEE 1
#include "./../../../linux/arch/arm/include/asm/mytee.h"

#define MAP_DEVICE0	MAP_REGION_FLAT(DEVICE0_BASE,			\
					DEVICE0_SIZE,			\
					MT_DEVICE | MT_RW | MT_SECURE)

#define MAP_SHARED_RAM	MAP_REGION_FLAT(SHARED_RAM_BASE,		\
					SHARED_RAM_SIZE,		\
					MT_DEVICE  | MT_RW | MT_SECURE)

#define MAP_MYTEE_DRAM0	MAP_REGION_FLAT(NS_DRAM0_BASE, NS_DRAM0_SIZE,	\
					MT_DEVICE | MT_RW | MT_SECURE)

#define MAP_NS_DRAM0	MAP_REGION_FLAT(NS_DRAM0_BASE, NS_DRAM0_SIZE,	\
					MT_MEMORY | MT_RW | MT_NS)

#define MAP_FIP		MAP_REGION_FLAT(PLAT_RPI3_FIP_BASE,		\
					PLAT_RPI3_FIP_MAX_SIZE,		\
					MT_MEMORY | MT_RO | MT_NS)

#define MAP_BL32_MEM	MAP_REGION_FLAT(BL32_MEM_BASE, BL32_MEM_SIZE,	\
					MT_MEMORY | MT_RW | MT_SECURE)

#ifdef SPD_opteed
#define MAP_OPTEE_PAGEABLE	MAP_REGION_FLAT(		\
				RPI3_OPTEE_PAGEABLE_LOAD_BASE,	\
				RPI3_OPTEE_PAGEABLE_LOAD_SIZE,	\
				MT_MEMORY | MT_RW | MT_SECURE)
#endif

/*
 * Table of regions for various BL stages to map using the MMU.
 */
#ifdef IMAGE_BL1
static const mmap_region_t plat_rpi3_mmap[] = {
	MAP_SHARED_RAM,
	MAP_DEVICE0,
	MAP_FIP,
#ifdef SPD_opteed
	MAP_OPTEE_PAGEABLE,
#endif
	{0}
};
#endif

#ifdef IMAGE_BL2
static const mmap_region_t plat_rpi3_mmap[] = {
	MAP_SHARED_RAM,
	MAP_DEVICE0,
	MAP_FIP,
	MAP_NS_DRAM0,
#ifdef BL32_BASE
	MAP_BL32_MEM,
#endif
	{0}
};
#endif

#ifdef IMAGE_BL31
static const mmap_region_t plat_rpi3_mmap[] = {
	MAP_SHARED_RAM,
	MAP_DEVICE0,
#ifdef CONFIG_MYTEE
	MAP_MYTEE_DRAM0,
#endif
#ifdef BL32_BASE
	MAP_BL32_MEM,
#endif
	{0}
};
#endif

/*******************************************************************************
 * Function that sets up the console
 ******************************************************************************/
static console_16550_t rpi3_console;

void rpi3_console_init(void)
{
	int rc = console_16550_register(PLAT_RPI3_UART_BASE,
					PLAT_RPI3_UART_CLK_IN_HZ,
					PLAT_RPI3_UART_BAUDRATE,
					&rpi3_console);
	if (rc == 0) {
		/*
		 * The crash console doesn't use the multi console API, it uses
		 * the core console functions directly. It is safe to call panic
		 * and let it print debug information.
		 */
		panic();
	}

	console_set_scope(&rpi3_console.console,
			  CONSOLE_FLAG_BOOT | CONSOLE_FLAG_RUNTIME);
}
//CONFIG_MYTEE
//1st table (1GB): 0x0f0f0000 -> maps from phy(0x40000000 ~)
//2nd table (2MB): 0x0f0f1000 -> maps from phy(0x0 ~ 0x3fffffff)
//page tables: 0x0f0f0000 ~ 0x0f0ff000
//hyp stack: 0x0f10e000~ 0x0f111000 (RESERVED)
//KBD protection: 0xf100000 ~ 0x0f103000 (RESERVED)
//MYTEE_ROBUF: PHY(0xE000000~0xE7FFFFF) (Reserved)
//HYP: PHY(0xE800000~0x100dffff): not mapped by s2 but mapped by shared pt. Regions for s2tables should not be mapped by shared pt as well
//EL3: virt(0x100E0000~0x100fffff) phy(identical mapping, 0x100E0000~)
//optee: phys(0x10100000-0x1012effff:txt, [0x1022e000-0x102fefff, 0x10200000-0x107fffff, 0x10800000-0x10ffffff]:rw) ==> 0xE800000~0x10ffffff 40mb should not be mapped


/*
0x0f0f0100~0x0f0f0148 is reserved address for saving information of allocating dynamic 4kb page
0x0f1f0000~0x0f1fa000 is reserved address for 4kb table
0x0f1fb000~0x0f1ff000 is DMA secure buffer
0x0f104000, 6000, 7000 -> DMA log page
*/

void setup_hyp_S2_tables(){
	unsigned long long  *addr_desc;
	unsigned long long long_desc;
	unsigned long long long_desc_na;
	unsigned long long na_mask = 0x3d; 	//NA

	addr_desc = (unsigned long long *) MYTEE_STAGE2_PAGE_TABLE_BASE_PHYS;
	*addr_desc = MYTEE_STAGE2_PAGE_TABLE_LEVEL1_1GB_PHYS | IS_TABLE_ENTRY; //2nd table address
	addr_desc++;
	*addr_desc = 0x40000000 | STAGE2_TRANSLATION_LEVEL012_LOWER_ATTRIBUTE_ACCESS_PERMISSION_TO_RW; //1GB mapping from 0x40000000
	addr_desc++;
	*addr_desc = 0x80000000 | STAGE2_TRANSLATION_LEVEL012_LOWER_ATTRIBUTE_ACCESS_PERMISSION_TO_RW; //1GB mapping from 0x80000000

	//creating 2nd table entries
	// 2MB * 512 = 1GB
	long_desc = STAGE2_TRANSLATION_LEVEL012_LOWER_ATTRIBUTE_ACCESS_PERMISSION_TO_RW;
	addr_desc = (unsigned long long *) MYTEE_STAGE2_PAGE_TABLE_LEVEL1_1GB_PHYS;
	for (int i =0; i < 512; i++){
		if ( i == 120){ 			//0x0f000000~: For secure kdb inbuf. We just set this 2MB as device memory	
			long_desc_na = long_desc & 0xffffff00;
			long_desc_na |= na_mask;
			*addr_desc = long_desc_na;
			long_desc += OFFSET_2MB;
			addr_desc++;
			
			continue;
		}
		else{
			*addr_desc = long_desc;
			long_desc += OFFSET_2MB;
			addr_desc++;
		}
	}
	
	// ************************ Frame Buffer Mapping ************************
	// fb_phy_mem : 0x3E402000 ~ 0x3EBEAFFF (8100KB, 0x7E9000 size)	
	// 0f0f1F90 = 3E4007FD			0x3E400000, 0x3E401000 (non-fb), [ 0x3E402000~ (fb)
	// 0f0f1F98 = 3E6007FD			0x3E600000 ~ 3E79FFFF (2MB) (fb)
	// 0f0f1FA0 = 3E8007FD			0x3E800000 ~ 3E99FFFF (2MB) (fb)
	// 0f0f1FA8 = 3EA007FD			..., 0x3EBEA000 (~0x3EBEAFFF) ], 0x3EBEB000 ... ~ 0x3EBFFFFF (non-fb)
	// 0f0f1FB0 = 3EC007FD
	
	// Splot 0x3E400000~ to 4KB pages 
	long_desc = STAGE2_TRANSLATION_LEVEL3_LOWER_ATTRIBUTE_ACCESS_PERMISSION_TO_RW; 		//page entry's last 2bits = 0b11
	long_desc += MYTEE_RPI3_FRAME_BUFFER_SPLIT_4KB_BASE_PHYS;
	addr_desc = (unsigned long long *) MYTEE_STAGE2_PAGE_TABLE_LEVEL2_2MB_FRAME_BUFFER_1ST_2MB_PHYS;
	for(int i = 0; i < 512; i++){		
		*addr_desc = long_desc;
		long_desc += OFFSET_4KB;
		addr_desc++;
	}
	addr_desc = (unsigned long long *) MYTEE_STAGE2_PAGE_TABLE_LEVEL1_1GB_PHYS + LEVEL2_2MB_FRAME_BUFFER_1ST_PHYS_OFFSET;
	*addr_desc = MYTEE_STAGE2_PAGE_TABLE_LEVEL2_2MB_FRAME_BUFFER_1ST_2MB_PHYS | IS_TABLE_ENTRY;	// Set 0x3E400000~ to 3rd page table
	
	// Splot 0x3EA00000~ to 4KB pages 
	long_desc = STAGE2_TRANSLATION_LEVEL3_LOWER_ATTRIBUTE_ACCESS_PERMISSION_TO_RW; 		//page entry's last 2bits = 0b11
	long_desc += MYTEE_RPI3_FRAME_BUFFER_ADDR_LAST_2MB_PHYS;
	addr_desc = (unsigned long long *) MYTEE_STAGE2_PAGE_TABLE_LEVEL2_2MB_FRAME_BUFFER_LAST_2MB_PHYS;
	for(int i = 0; i < 512; i++){		
		*addr_desc = long_desc;
		long_desc += OFFSET_4KB;
		addr_desc++;
	}
	addr_desc = (unsigned long long *) MYTEE_STAGE2_PAGE_TABLE_LEVEL1_1GB_PHYS + LEVEL2_2MB_FRAME_BUFFER_LAST_PHYS_OFFSET;
	*addr_desc = MYTEE_STAGE2_PAGE_TABLE_LEVEL2_2MB_FRAME_BUFFER_LAST_2MB_PHYS | IS_TABLE_ENTRY;	// Set 0x3E400000~ to 3rd page table
	

	// ************************ Create Fake Frame Buffer ************************
	// fake_fb : 0xE800000 ~ 0xEFFFFFF
	// Split first 2MB and last 2MB to 4kb
	long_desc = STAGE2_TRANSLATION_LEVEL3_LOWER_ATTRIBUTE_ACCESS_PERMISSION_TO_RW; 		//page entry's last 2bits = 0b11
	long_desc += MYTEE_FAKE_FRAME_BUFFER_BASE_PHYS;
	addr_desc = (unsigned long long *) MYTEE_STAGE2_PAGE_TABLE_LEVEL2_2MB_FAKE_FRAME_BUFFER_1ST_2MB_PHYS;
	for(int i = 0; i < 512; i++){		
		*addr_desc = long_desc;
		long_desc += OFFSET_4KB;
		addr_desc++;
	}

	long_desc = STAGE2_TRANSLATION_LEVEL3_LOWER_ATTRIBUTE_ACCESS_PERMISSION_TO_RW; 		//page entry's last 2bits = 0b11
	long_desc += MYTEE_FAKE_FRAME_BUFFER_LAST_PHYS;
	addr_desc = (unsigned long long *) MYTEE_STAGE2_PAGE_TABLE_LEVEL2_2MB_FAKE_FRAME_BUFFER_LAST_2MB_PHYS;
	for(int i = 0; i < 512; i++){		
		*addr_desc = long_desc;
		long_desc += OFFSET_4KB;
		addr_desc++;
	}
	
	// 8100KB - 4MB(2MB Mapping * 2) = 4004KB
	// 4M - 4004KB = 92KB = the number of not_fb page is 23
	
	long_desc = STAGE2_TRANSLATION_LEVEL3_LOWER_ATTRIBUTE_ACCESS_PERMISSION_TO_RW; 		//page entry's last 2bits = 0b11
	long_desc += MYTEE_RPI3_FRAME_BUFFER_SPLIT_4KB_BASE_PHYS;
	addr_desc = (unsigned long long *) MYTEE_STAGE2_PAGE_TABLE_LEVEL2_2MB_FAKE_FRAME_BUFFER_1ST_2MB_PHYS;
	for(int i = 0; i < 2; i++){		
		*addr_desc = long_desc;
		long_desc += OFFSET_4KB;
		addr_desc++;
	}
		
	long_desc = STAGE2_TRANSLATION_LEVEL3_LOWER_ATTRIBUTE_ACCESS_PERMISSION_TO_RW; 		//page entry's last 2bits = 0b11
	long_desc += MYTEE_RPI3_FRAME_BUFFER_SPLIT_4KB_NOT_FRAME_BUFFER_START_PHYS;
	addr_desc = (unsigned long long *) MYTEE_STAGE2_PAGE_TABLE_LEVEL2_2MB_FAKE_FRAME_BUFFER_LAST_2MB_PHYS + MYTEE_STAGE2_PAGE_TABLE_LEVEL3_4KB_NOT_FRAME_BUFFER_PHYS_OFFSET;
	for(int i = 0; i < 21; i++){		
		*addr_desc = long_desc;
		long_desc += OFFSET_4KB;
		addr_desc++;
	}
	
	// ***********************DMA controller(BCM) ********************//
	// For RPI3 start from 0x3F007000 (FIXED-CONSTANT ADDRESS)
	// 0xf0f1fc0 is entry address for 0x3f007000 in 2nd table
	long_desc = STAGE2_TRANSLATION_LEVEL3_LOWER_ATTRIBUTE_ACCESS_PERMISSION_TO_RW; 		//page entry's last 2bits = 0b11
	long_desc += MYTEE_RPI3_DMA_CONTROLLER_SPLIT_4KB_BASE_PHYS;
	addr_desc = (unsigned long long *) MYTEE_STAGE2_PAGE_TABLE_LEVEL2_2MB_DMA_PHYS;
	for(int i = 0; i < 512; i++){		
//		if(i == 7) 		// DMA
//			long_desc &= 0xffffff7f;
		*addr_desc = long_desc;
		long_desc += OFFSET_4KB;
				
		addr_desc++;
	}
	
	addr_desc = (unsigned long long *) MYTEE_STAGE2_PAGE_TABLE_LEVEL1_1GB_PHYS + LEVEL2_2MB_DMA_PHYS_OFFSET;	// entry address for 0x3f007000
	*addr_desc = MYTEE_STAGE2_PAGE_TABLE_LEVEL2_2MB_DMA_PHYS | IS_TABLE_ENTRY;	// Set first 2MB to 3rd page table

	// ****************USB internal DMA controller(DWC-otg)****************//
	// For RPI3 start from 0x3F980000 (FIXED-CONSTANT ADDRESS)
	// 0xf0f1fe0 is entry address for 0x3f980000 in 2nd table
	long_desc = STAGE2_TRANSLATION_LEVEL3_LOWER_ATTRIBUTE_ACCESS_PERMISSION_TO_RW; 		//page entry's last 2bits = 0b11
	long_desc += MYTEE_RPI3_USB_INTERNAL_DMA_CONTROLLER_SPLIT_4KB_BASE_PHYS;
	addr_desc = (unsigned long long *) MYTEE_STAGE2_PAGE_TABLE_LEVEL2_2MB_USB_INTERNAL_DMA_PHYS;
	for(int i = 0; i < 512; i++){		
//		if(i == 7) 		// DMA
//			long_desc &= 0xffffff7f;
		*addr_desc = long_desc;
		long_desc += OFFSET_4KB;
				
		addr_desc++;
	}
	
	addr_desc = (unsigned long long *) (MYTEE_STAGE2_PAGE_TABLE_LEVEL1_1GB_PHYS + LEVEL2_2MB_USB_INTERNAL_DMA_PHYS_OFFSET);	// entry address for 0x3f007000
	*addr_desc = MYTEE_STAGE2_PAGE_TABLE_LEVEL2_2MB_USB_INTERNAL_DMA_PHYS | IS_TABLE_ENTRY;	// Set first 2MB to 3rd page table


	//Initialize some hyp data page
	//0xf100000: inbuf address (secure kbd)
	//0xf101000: TA event buffer address (secure kbd)
	//0xf102000: Secure KBD related variables
	
	int * page = (int *)MYTEE_HYP_DATA_PAGE_BASE_PHYS;
	for(int i=0; i < 0x100000; i++)
		*page++ = 0;

	// ****************dynamic MMIO 4kb s2 page table information****************//
	// 0x0f0f0100~0x0f0f0148 is reserved address for saving information of allocating dynamic 4kb page
	long_desc = 0x0;
	addr_desc = (unsigned long long *) MYTEE_HYP_DYNAMICALLY_ALLOCATED_STAGE2_PAGE_TABLE_4KB_INFORM_PHYS;
	for(int i = 0; i < 10; i++){		
		*addr_desc = long_desc;				
		addr_desc++;
	}
	
	/*
	// trusted keyboard flag
	long_desc = 0x0;
	addr_desc = (unsigned long long *) MYTEE_TRUSTED_KEYBOARD_FLAG_PHYS;
	*addr_desc = long_desc;
	*/
	
	//  ****************dynamic MMIO 4kb s2 page table           ****************
	// 0x0f1f0000~0x0f1fa000 is reserved address for 4kb table
	long_desc = 0x0;
	addr_desc = (unsigned long long *) MYTEE_HYP_DYNAMICALLY_ALLOCATED_STAGE2_PAGE_TABLE_4KB_PHYS;
	for(int i = 0; i < 512*10; i++){		
		*addr_desc = long_desc;				
		addr_desc++;
	}
	
	//  ****************secure buffer for verifying DMA Control blocks           ****************
	// 0x0f1fa000~0x0f1fb000 is reserved address for copying CB in kernel space
	long_desc = 0x0;
	addr_desc = (unsigned long long *) MYTEE_HYP_SECURE_BUFFER_FOR_VERIFYTING_DMA_CONTROL_BLOKCS_PHYS;
	for(int i = 0; i < 512*4; i++){		
		*addr_desc = long_desc;				
		addr_desc++;
	}
	
	// ****************Logging cb address to avoid infinite loop          ****************
	// 0x0fa00000~0x0fa04000 is reserved address for copying CB in user space
	long_desc = 0x0;
	addr_desc = (unsigned long long *) MYTEE_HYP_LOGGING_CONTROL_BLOCKS_NEXT_CB_ADDRESS_PHYS;
	for(int i = 0; i < 512*2; i++){		
		*addr_desc = long_desc;				
		addr_desc++;
	}
	
	// ****************		To save devnum of HID keyboard          ****************
	// 0x0f102000 is reserved address for devnum
	long_desc = 0xFFFFFFFF;
	addr_desc = (unsigned long long *) MYTEE_HYP_SECURE_HID_KEYBOARD_DEVNUM_SAVE_PHYS;
	*addr_desc = long_desc;				
}

/*******************************************************************************
 * Function that sets up the translation tables.
 ******************************************************************************/
void rpi3_setup_page_tables(uintptr_t total_base, size_t total_size,
			    uintptr_t code_start, uintptr_t code_limit,
			    uintptr_t rodata_start, uintptr_t rodata_limit
#if USE_COHERENT_MEM
			    , uintptr_t coh_start, uintptr_t coh_limit
#endif
			    )
{
	/*
	 * Map the Trusted SRAM with appropriate memory attributes.
	 * Subsequent mappings will adjust the attributes for specific regions.
	 */
	VERBOSE("Trusted SRAM seen by this BL image: %p - %p\n",
		(void *) total_base, (void *) (total_base + total_size));
	mmap_add_region(total_base, total_base,
			total_size,
			MT_MEMORY | MT_RW | MT_SECURE);

	/* Re-map the code section */
	VERBOSE("Code region: %p - %p\n",
		(void *) code_start, (void *) code_limit);
	mmap_add_region(code_start, code_start,
			code_limit - code_start,
			MT_CODE | MT_SECURE);

	/* Re-map the read-only data section */
	VERBOSE("Read-only data region: %p - %p\n",
		(void *) rodata_start, (void *) rodata_limit);
	mmap_add_region(rodata_start, rodata_start,
			rodata_limit - rodata_start,
			MT_RO_DATA | MT_SECURE);

#if USE_COHERENT_MEM
	/* Re-map the coherent memory region */
	VERBOSE("Coherent region: %p - %p\n",
		(void *) coh_start, (void *) coh_limit);
	mmap_add_region(coh_start, coh_start,
			coh_limit - coh_start,
			MT_DEVICE | MT_RW | MT_SECURE);
#endif

	mmap_add(plat_rpi3_mmap);
	setup_hyp_S2_tables(); //CONFIG_MYTEE
	init_xlat_tables();
//MYTEE
        //@@@Map phy IN EL3
	
	unsigned long long * addr_desc;
	unsigned long long long_desc;
	//@@MAP PHY 0X0~ 0X10000000
        addr_desc = (unsigned long long *) MYTEE_EL3_LEVEL1_1GB_PHYS;
        long_desc = 0x0000000000000761; //(ng bit off) no screen. boot ok

       // long_desc = 0x0000000000000741;  //(non secure==0) screen. boot ok
    	for (int i=0; i < 0x7f; i++){
                *addr_desc++ = long_desc;
                long_desc+= OFFSET_2MB;
        }
	//@@MAP PHY 0X12000000~ 0X30000000
        addr_desc = (unsigned long long *) (MYTEE_EL3_LEVEL1_1GB_PHYS | 0x480);
        long_desc = 0x0000000012000761; 
    	for (int i=0; i < 0xf0; i++){
                *addr_desc++ = long_desc;
                long_desc+= OFFSET_2MB;
        }


        //@@@Map phy 0x30000000 ~ 0x3f000000 to 0x100f1000.(offset: c00)

        addr_desc = (unsigned long long *) (MYTEE_EL3_LEVEL1_1GB_PHYS | 0xc00);
        long_desc = 0x0000000030000761; 
    	for (int i=0; i < 0x78; i++){
                *addr_desc++ = long_desc;
                long_desc+= OFFSET_2MB;

        }

}

/*******************************************************************************
 * Return entrypoint of BL33.
 ******************************************************************************/
uintptr_t plat_get_ns_image_entrypoint(void)
{
#ifdef PRELOADED_BL33_BASE
	return PRELOADED_BL33_BASE;
#else
	return PLAT_RPI3_NS_IMAGE_OFFSET;
#endif
}

/*******************************************************************************
 * Gets SPSR for BL32 entry
 ******************************************************************************/
uint32_t rpi3_get_spsr_for_bl32_entry(void)
{
	/*
	 * The Secure Payload Dispatcher service is responsible for
	 * setting the SPSR prior to entry into the BL32 image.
	 */
	return 0;
}

/*******************************************************************************
 * Gets SPSR for BL33 entry
 ******************************************************************************/
uint32_t rpi3_get_spsr_for_bl33_entry(void)
{
#if RPI3_BL33_IN_AARCH32
	INFO("BL33 will boot in Non-secure AArch32 Hypervisor mode\n");
	return SPSR_MODE32(MODE32_hyp, SPSR_T_ARM, SPSR_E_LITTLE,
			   DISABLE_ALL_EXCEPTIONS);
#else
	return SPSR_64(MODE_EL2, MODE_SP_ELX, DISABLE_ALL_EXCEPTIONS);
#endif
}

unsigned int plat_get_syscnt_freq2(void)
{
	return SYS_COUNTER_FREQ_IN_TICKS;
}

uint32_t plat_ic_get_pending_interrupt_type(void)
{
	ERROR("rpi3: Interrupt routed to EL3.\n");
	return INTR_TYPE_INVAL;
}

uint32_t plat_interrupt_type_to_line(uint32_t type, uint32_t security_state)
{
	assert((type == INTR_TYPE_S_EL1) || (type == INTR_TYPE_EL3) ||
	       (type == INTR_TYPE_NS));

	assert(sec_state_is_valid(security_state));

	/* Non-secure interrupts are signalled on the IRQ line always. */
	if (type == INTR_TYPE_NS)
		return __builtin_ctz(SCR_IRQ_BIT);

	/* Secure interrupts are signalled on the FIQ line always. */
	return  __builtin_ctz(SCR_FIQ_BIT);
}
