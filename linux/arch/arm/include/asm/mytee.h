#ifdef CONFIG_MYTEE

#ifndef __ASSEMBLY__
typedef unsigned char u8;
typedef unsigned int u32;
#define MYTEE_MMIO_CONTEXT_DEVICE_TPM 1			
#define MYTEE_MMIO_CONTEXT_DEVICE_FRAMEBUFFER 2
#define MYTEE_MMIO_CONTEXT_DEVICE_KEYBOARD 3

#define MYTEE_IOCTL_REQUEST_UNSHIELD_SPI 0x10
#define MYTEE_IOCTL_REQUEST_START_DMA_VERIFY 0x11
#define MYTEE_IOCTL_REQUEST_UNSHIELD_FB_AND_MBOX_MMAP 0x12
#define MYTEE_IOCTL_REQUEST_UNSHIELD_FB_WRITE 0x13

#define BCM2835_VIRT_TO_BUS_OFFSET 0x40000000

/* stage 2 page table addr */
#define MYTEE_STAGE2_PAGE_TABLE_BASE_PHYS 0x0F0F0000					//  Each entry maps 1GB (1GB * 4 = 4GB)
	/* stage 2 page table, split first 1GB to 2MB blocks for the FB */
	#define MYTEE_STAGE2_PAGE_TABLE_LEVEL1_1GB_PHYS 0x0F0F1000			// Each entry maps 2MB (2MB * 512 = 1GB)
	#define LEVEL2_2MB_FRAME_BUFFER_1ST_PHYS_OFFSET 0xF90				// 0x0F0F1F90 hols next table entry: 0F0F3003
	#define LEVEL2_2MB_FRAME_BUFFER_LAST_PHYS_OFFSET 0xFA8				// 0x0F0F1FA8 holds next table entry: 0F0F4003
		/* stage 2 page table.  Split 2MB block to 4KB pages for the FB*/				// Each entry maps 4KB (4KB * 512 = 2MB)
		#define MYTEE_STAGE2_PAGE_TABLE_LEVEL2_2MB_FRAME_BUFFER_1ST_2MB_PHYS 0x0F0F3000
		#define MYTEE_STAGE2_PAGE_TABLE_LEVEL2_2MB_FRAME_BUFFER_LAST_2MB_PHYS 0x0F0F4000
		
		#define MYTEE_STAGE2_PAGE_TABLE_LEVEL3_4KB_NOT_FRAME_BUFFER_PHYS_OFFSET 0xF58
		#define MYTEE_STAGE2_PAGE_TABLE_LEVEL2_2MB_FAKE_FRAME_BUFFER_1ST_2MB_PHYS 0x0F0F5000
		#define MYTEE_STAGE2_PAGE_TABLE_LEVEL2_2MB_FAKE_FRAME_BUFFER_LAST_2MB_PHYS 0x0F0F6000	// Entries beyond 0x0F0F6F58 do not map the framebuffer
		
	/* stage 2 page table, split first 1GB to 2MB blocks for the DMA */
	#define LEVEL2_2MB_DMA_PHYS_OFFSET 0xFC0
		/* stage 2 page table, split 2MB block to 4KB pages for the DMA*/
		#define MYTEE_STAGE2_PAGE_TABLE_LEVEL2_2MB_DMA_PHYS 0x0F0F9000
	/* stage 2 page table, split first 1GB to 2MB for the USB INTERNAL DMA */
	#define LEVEL2_2MB_USB_INTERNAL_DMA_PHYS_OFFSET 0xFE0
		/* stage 2 page table, split 2MB to 4KB pages for the USB INTERNAL DMA*/
		#define MYTEE_STAGE2_PAGE_TABLE_LEVEL2_2MB_USB_INTERNAL_DMA_PHYS 0x0F0FA000
		
		/* example */
		#define MYTEE_STAGE2_PAGE_TABLE_LEVEL2_2MB_PHYS 0x0F0F2000
		
#define OFFSET_2MB 0x200000
#define OFFSET_4KB 0x1000
#define IS_TABLE_ENTRY 0x3
#define IS_BLOCK_ENTRY 0x1
#define STAGE2_TRANSLATION_LEVEL012_LOWER_ATTRIBUTE_ACCESS_PERMISSION_TO_RW 0x7FD
#define STAGE2_TRANSLATION_LEVEL012_LOWER_ATTRIBUTE_ACCESS_PERMISSION_TO_NA 0x73D
#define STAGE2_TRANSLATION_LEVEL3_LOWER_ATTRIBUTE_ACCESS_PERMISSION_TO_RW 0x7FF
#define STAGE2_TRANSLATION_LEVEL3_LOWER_ATTRIBUTE_ACCESS_PERMISSION_TO_NA 0x73F

/* Physical address of the frame buffer and fake frame buffer*/
#define MYTEE_RPI3_FRAME_BUFFER_ADDR_BASE_PHYS 0x3E402000
#define MYTEE_RPI3_FRAME_BUFFER_ADDR_MAX_SIZE 0x7E9000
#define MYTEE_RPI3_FRAME_BUFFER_ADDR_2ND_2MB_PHYS 0x3E600000
#define MYTEE_RPI3_FRAME_BUFFER_ADDR_3RD_2MB_PHYS 0x3E800000
#define MYTEE_RPI3_FRAME_BUFFER_ADDR_LAST_2MB_PHYS 0x3EA00000
#define MYTEE_RPI3_FRAME_BUFFER_SPLIT_4KB_BASE_PHYS 0x3E400000
#define MYTEE_RPI3_FRAME_BUFFER_SPLIT_4KB_NOT_FRAME_BUFFER_START_PHYS 0x3EBEB000
#define MYTEE_FAKE_FRAME_BUFFER_BASE_PHYS 0xE800000
#define MYTEE_FAKE_FRAME_BUFFER_2ND_PHYS 0xEA00000
#define MYTEE_FAKE_FRAME_BUFFER_3RD_PHYS 0xEC00000
#define MYTEE_FAKE_FRAME_BUFFER_LAST_PHYS 0xEE00000

/* Physical address of the DMA controller (BCM) */
#define MYTEE_RPI3_DMA_CONTROLLER_SPLIT_4KB_BASE_PHYS 0x3F000000

/* Physical address of the USB internal DMA controller (DWC-otg) */
#define MYTEE_RPI3_USB_INTERNAL_DMA_CONTROLLER_SPLIT_4KB_BASE_PHYS 0x3F800000

/* Virtual address of the hypervisor */
	/* Trusted TPM relevant */
#define MYTEE_HYP_SECURE_SPI_TPM_RX_BUF_VIRT 0x8F108000
#define MYTEE_HYP_SECURE_SPI_TPM_TX_BUF_VIRT 0x8F107000
#define MYTEE_HYP_SECURE_TPM_LOG_PAGE_VIRT 0x8F104000
#define MYTEE_HYP_SECURE_BCM2835_SPI_REGS_SAVE_VIRT 0x8F106000
#define MYTEE_HYP_SECURE_BCM2835_SPI_SAVE_VIRT 0x8f105000
#define MYTEE_TRUSTED_TPM_FLAG_VIRT 0x8F105F00
#define MYTEE_TRUSTED_TPM_REQUEST_COUNT_VIRT 0x8F105F08
#define MYTEE_HYP_SECURE_SPI_TPM_RX_BOOL_VIRT 0x8F105F10
#define MYTEE_HYP_SECURE_SPI_TPM_TX_BOOL_VIRT 0x8F105F18
	/* Trusted Keyboard relevant*/
#define MYTEE_HYP_SECURE_HID_KEYBOARD_DEVNUM_SAVE_VIRT 0x8f102000
#define MYTEE_HYP_SECURE_HID_KEYBOARD_INBUF_VIRT 0x8F100000
#define MYTEE_HYP_SECURE_KEYBOARD_BUF_VIRT 0x8F101000
#define MYTEE_HYP_SECURE_KEYBOARD_BUF_INDEX_VIRT 0x8F102040
#define MYTEE_TRUSTED_KEYBOARD_FLAG_VIRT 0x8F102030
	/* Trusted FB relevant */
#define MYTEE_TRUSTED_FB_MMAP_FLAG_VIRT 0x8F10B000
#define MYTEE_TRUSTED_FB_WRITE_FLAG_VIRT 0x8F10B008
#define MYTEE_TRUSTED_FB_VERIFY_RET_VIRT 0x8F10B010
#define MYTEE_TRUSTED_FB_CONTEXT_VIRT 0x8F10B018
#define MYTEE_TRUSTED_FB_TEMP_VIRT 0x8F10B020
#define MYTEE_TRUSTED_FB_MMAP_LOG_VIRT 0x8F10A000
#define MYTEE_TRUSTED_FB_WRITE_LOG_VIRT 0x8F10C000
#define MYTEE_TRUSTED_FB_WRITE_SECURE_SRC_VIRT 0x8F200000
#define MYTEE_TRUSTED_FB_MAIL_BOX_SETTING_COUNT_VIRT 0x8F10D000
#define MYTEE_TRUSTED_FB_MAIL_BOX_LOG_CONTEXT_VIRT 0x8F10D004
#define MYTEE_TRUSTED_FB_MAIL_BOX_LOG_PAGE_VIRT 0x8F10D008
#define MYTEE_HYP_SECURE_MAIL_BOX_MSG_VIRT 0x8f109000
#define MYTEE_TRUSTED_FB_MEM_ADDR_VIRT 0x8F10B028

/* Physical address of the hypervisor */
#define MYTEE_HYP_DATA_PAGE_BASE_PHYS 0x0F100000
#define MYTEE_HYP_DYNAMICALLY_ALLOCATED_STAGE2_PAGE_TABLE_4KB_PHYS 0x0F1F0000
#define MYTEE_HYP_DYNAMICALLY_ALLOCATED_STAGE2_PAGE_TABLE_4KB_INFORM_PHYS 0x0F0F0100

	/* Trusted TPM relevant */
#define MYTEE_HYP_SECURE_SPI_TPM_RX_BUF_PHYS 0x0F108000	
#define MYTEE_HYP_SECURE_TPM_LOG_PAGE_PHYS 0x0F104000
#define MYTEE_TRUSTED_TPM_FLAG_PHYS 0x0F105F00
	/* Trusted Keyboard relevant */
#define MYTEE_HYP_SECURE_KEYBOARD_BUF_PHYS 0x0F101000
#define MYTEE_HYP_SECURE_KEYBOARD_BUF_INDEX_TA_PHYS 0x0F102050
#define MYTEE_TRUSTED_KEYBOARD_FLAG_PHYS 0x0F102030	
#define MYTEE_HYP_SECURE_HID_KEYBOARD_DEVNUM_SAVE_PHYS 0x0f102000
	/* Trusted FB relevant */
#define MYTEE_TRUSTED_FB_MMAP_FLAG_PHYS 0x0F10B000
#define MYTEE_TRUSTED_FB_WRITE_FLAG_PHYS 0x0F10B008
#define MYTEE_TRUSTED_FB_MAIL_BOX_SETTING_COUNT_PHYS 0x0F10D000
#define MYTEE_TRUSTED_FB_WRITE_SECURE_SRC_PHYS 0x0F200000

	/* DMA filter relevant */
#define MYTEE_HYP_SECURE_BUFFER_FOR_VERIFYTING_DMA_CONTROL_BLOKCS_PHYS 0x0F1FA000
#define MYTEE_HYP_LOGGING_CONTROL_BLOCKS_NEXT_CB_ADDRESS_PHYS 0x0FA00000

#define MYTEE_CONTEXT_FB_MMAP 0x0
#define MYTEE_CONTEXT_FB_WRITE 0x1
#define MYTEE_CONTEXT_MAILBOX_MMAP 0xF2
#define MYTEE_CONTEXT_MAILBOX_WRITE 0xF3
#define MYTEE_CONTEXT_MBOX_LOG_COMPLETE 0x100



/* For verification */
#define KERNEL_TEXT_PHYS_START 0x00200000
#define KERNEL_TEXT_PHYS_END 0x00a00000
#define HYP_EL3_OPTEE_PHYS_START 0x0E800000
#define HYP_EL3_OPTEE_PHYS_END 0x11000000



/* Physical address of translation table for TTBR0_EL3*/
#define MYTEE_EL3_LEVEL1_1GB_PHYS 0x100F1000

/* Active monitor relevant */
#define MYTEE_EXTRA_MEM_START    0xDA00000
#define MYTEE_EXTRA_MEM_SIZE     0x600000

#define   MYTEE_ROBUF_START      0xE000000
#define   MYTEE_ROBUF_SIZE       0x800000
#define   MYTEE_PGT_BITMAP_LEN 	 0x40000

#define MYTEE_EXTRA_MEM_VA (phys_to_virt(MYTEE_EXTRA_MEM_START))
#define MYTEE_RBUF_VA      (phys_to_virt(MYTEE_ROBUF_START))
#define RO_PAGES  (MYTEE_ROBUF_SIZE >> PAGE_SHIFT) 

#define EMUL_NEW_PGD 14
#define EMUL_PGD_FREE 15
#define EMUL_RO_MEMSET 16
#define EMUL_RO_MEMCPY 17

#define MYTEE_INIT 22

#define BLANK_PAGE_MAP_ADDR 0x0f130000
#define BLANK_PAGE_PHY_ADDR_OFFSET 0x8
#define PGT_BITMAP_ADDR 0x0f131000

/* Hypervisor (EL2) services */
#define MYTEE_SET_VTCR	96
#define MYTEE_UP_PRIV 99
#define MYTEE_DOWN_PRIV 100
#define MYTEE_MEMCOPY 101
#define MYTEE_USBHID_INBUF_DMA_SAVE 107
#define MYTEE_LAZY_CPU1_SETUP 120

/* Active monitor(EL3) services*/
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

extern u8 mytee_pgt_bitmap[];
//extern u8 mytee_map_bitmap[];

extern u8 mytee_init_flag;

extern void *mytee_ro_alloc(void);
extern void mytee_ro_free(void *free_addr);
extern unsigned int is_mytee_ro_page(u32 addr);
extern unsigned long spi_mmio_virtaddr;
extern unsigned long mbox_mmio_virtaddr;
extern unsigned long fb_phys_addr;
extern unsigned long fb_size;
extern int is_trusted_display;

void mytee_enable_hyp_mmu(u32 hypercall, u32 ttbr_low, u32 ttbr_high); // Setup page table, exception vector, and HSCTLR

void mytee_kernel_text_ro(u32 hypercall, u32 phys_text_start, u32 phys_text_end, u32 temp);
void mytee_memcopy(u32 hypercall, u8 *iobuf_phy, u8 transfer_len, u8* in);
void mytee_up_priv(u32 hypercall, u32 dummy, u32 dummy2,u32 dummy3);
void mytee_down_priv(u32 hypercall, u32 dummy);
void mytee_set_httbr_ttbr1(u32 hypercall, u32 dummy);
void mytee_set_httbr_ttbr0(u32 hypercall, u32 dummy, u32 ttbr0);
void mytee_unmmap_fb(u32 hypercall, u32 dummy, u32 dummy2);
void mytee_remmap_fb(u32 hypercall, u32 dummy, u32 dummy2);
//void mytee_tpm_memset_noaccess(u32 hypercall, u32 mytee_iobuf, u32 dummy, u32 dummy2);
void mytee_lazy_cpu1_setup(u32 hypercall, u32 dummy);
//void mytee_tpm_memset_4kb(u32 hypercall, u32 mytee_iobuf, u32 dummy, u32 dummy2);
//void mytee_tpm_memset_na(u32 hypercall, u32 mytee_iobuf);
//void mytee_tpm_memset_rw(u32 hypercall, u32 mytee_iobuf);
void mytee_shield_mmio(u32 hypercall, u32 virtaddr_mmio, u32 dummy, u32 shield_size);
void mytee_unshield_mmio(u32 hypercall, u32 virtaddr_mmio, u32 dummy, u32 shield_size);
void mytee_shield_mmio_4kb(u32 hypercall, u32 virtaddr_mmio, u32 mmio_device, u32 shield_size);
void mytee_unshield_mmio_4kb(u32 hypercall, u32 virtaddr_mmio, u32 mmio_device, u32 shield_size);
void mytee_dma_set_non_cacheable(u32 hypercall, u32 dummy, u32 dummy2, u32 dummy3);
void mytee_shield_mmio_with_phys(u32 hypercall, u32 physaddr_mmio, u32 dummy, u32 shield_size);
void mytee_unshield_mmio_with_phys(u32 hypercall, u32 physaddr_mmio, u32 dummy, u32 shield_size);
int mytee_get_kernel_phys(u32 hypercall, u32 virt_addr, u32 dummy, u32 dummy2);
void mytee_usbhid_inbuf_dma_save(u32 hypercall, u32 usbhid_inbuf, u32 dummy, u32 dummy2);

void mytee_new_pgd(u32 cmd_id, u32 pgd_addr, u32 dummy, u32 dummy2, u32 dummy3, u32 dummy4);
void mytee_pgd_free(u32 cmd_id, u32 pgd_addr, u32 dummy, u32 dummy2, u32 dummy3, u32 dummy4);

void mytee_ro_memset(u32 cmd_id, u32 target, u32 value, u32 size, u32 dummy, u32 dummy2);
void mytee_ro_memcpy(u32 cmd_id, u32 target, u32 source, u32 size, u32 dummy, u32 dummy2);

void mytee_init_monitor(u32 cmd_id, u32 mytee_extra_mem_pgt_pa, u32 mytee_pgt_bitmap_addr, u32 dummy3);

#else

#endif

#endif
