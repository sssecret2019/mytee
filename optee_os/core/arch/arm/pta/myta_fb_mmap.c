/*
 * Copyright (c) 2016, Linaro Limited
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <tee_internal_api.h>
#include <tee_internal_api_extensions.h>
#include <kernel/pseudo_ta.h>
#include <string.h>
#include <mm/core_mmu.h>
#include <mm/core_memprot.h>
#include <myta_fb_mmap.h>
#include <kernel/tee_time.h>
unsigned int wait_count_mmap=0;

struct fb_bitfield {
	unsigned long offset;			/* beginning of bitfield	*/
	unsigned long length;			/* length of bitfield		*/
	unsigned long msb_right;		/* != 0 : Most significant bit is */ 
					/* right */ 
};

struct fb_var_screeninfo {
	unsigned long xres;			/* visible resolution		*/
	unsigned long yres;
	unsigned long xres_virtual;		/* virtual resolution		*/
	unsigned long yres_virtual;
	unsigned long xoffset;			/* offset from virtual to visible */
	unsigned long yoffset;			/* resolution			*/

	unsigned long bits_per_pixel;		/* guess what			*/
	unsigned long grayscale;		/* 0 = color, 1 = grayscale,	*/
					/* >1 = FOURCC			*/
	struct fb_bitfield red;		/* bitfield in fb mem if true color, */
	struct fb_bitfield green;	/* else only length is significant */
	struct fb_bitfield blue;
	struct fb_bitfield transp;	/* transparency			*/	

	unsigned long nonstd;			/* != 0 Non standard pixel format */

	unsigned long activate;			/* see FB_ACTIVATE_*		*/

	unsigned long height;			/* height of picture in mm    */
	unsigned long width;			/* width of picture in mm     */

	unsigned long accel_flags;		/* (OBSOLETE) see fb_info.flags */

	/* Timing: All values in pixclocks, except pixclock (of course) */
	unsigned long pixclock;			/* pixel clock in ps (pico seconds) */
	unsigned long left_margin;		/* time from sync to picture	*/
	unsigned long right_margin;		/* time from picture to sync	*/
	unsigned long upper_margin;		/* time from sync to picture	*/
	unsigned long lower_margin;
	unsigned long hsync_len;		/* length of horizontal sync	*/
	unsigned long vsync_len;		/* length of vertical sync	*/
	unsigned long sync;			/* see FB_SYNC_*		*/
	unsigned long vmode;			/* see FB_VMODE_*		*/
	unsigned long rotate;			/* angle we rotate counter clockwise */
	unsigned long colorspace;		/* colorspace for FOURCC-based modes */
	unsigned long reserved[4];		/* Reserved for future compatibility */
} fb_vscreeninfo_log_fb_mmap;

static TEE_Result set_up_trusted_display_flag_on(uint32_t param_types,
	TEE_Param params[4])
{
	
	struct fb_var_screeninfo* framebuffer_variable_screeninfo = (struct fb_var_screeninfo*)params[0].memref.buffer;
	const unsigned long trusted_display_mmap_flag_phys = MYTEE_TRUSTED_FB_MMAP_FLAG_PHYS;
	uint32_t* trusted_flag_virt;
	bool ret;
	
	// Set values of framebuffer and log data
	fb_vscreeninfo_log_fb_mmap.xres = framebuffer_variable_screeninfo->xres=0x780;
	fb_vscreeninfo_log_fb_mmap.yres = framebuffer_variable_screeninfo->yres=0x438;
	fb_vscreeninfo_log_fb_mmap.xres_virtual = framebuffer_variable_screeninfo->xres_virtual=0x780;
	fb_vscreeninfo_log_fb_mmap.yres_virtual = framebuffer_variable_screeninfo->yres_virtual=0x438;
	fb_vscreeninfo_log_fb_mmap.bits_per_pixel = framebuffer_variable_screeninfo->bits_per_pixel=0x20;
	fb_vscreeninfo_log_fb_mmap.xoffset = framebuffer_variable_screeninfo->xoffset=0x0;
	fb_vscreeninfo_log_fb_mmap.yoffset = framebuffer_variable_screeninfo->yoffset=0x0;

	//IMSG("trusted display flag on");
	ret = core_mmu_add_mapping(MEM_AREA_IO_NSEC, trusted_display_mmap_flag_phys, 0x00001000);
	if(ret!=0x1){
		return TEE_ERROR_BAD_STATE;
	}
	trusted_flag_virt = phys_to_virt (trusted_display_mmap_flag_phys, MEM_AREA_IO_NSEC);
	*trusted_flag_virt = 1;
	
	return TEE_SUCCESS;
}

static TEE_Result mmap_ta_and_trusted_display_start(uint32_t param_types,
	TEE_Param params[4])
{
	int* fb_info_ptr = (int*)params[0].memref.buffer;
	unsigned long mmapLog_phys = FBMMAPLOG;
	unsigned long* mmapLog_virt;
	const unsigned long setting_count_phys = MYTEE_TRUSTED_FB_MAIL_BOX_SETTING_COUNT_PHYS;
	unsigned long* log_page_virtaddr;
	uint32_t* setting_count_virt;
	uint32_t* context;
	const unsigned long s2_4kb_table_info_phys = MYTEE_HYP_DYNAMICALLY_ALLOCATED_STAGE2_PAGE_TABLE_4KB_INFORM_PHYS;
	unsigned long* s2_4kb_table_info_virt;
	char* framebuffer_virt;
	bool ret;

	int fb_info[6];
	memcpy(fb_info, fb_info_ptr, params[1].memref.size);
	int width  = fb_info[0];  
	int height = fb_info[1];
	int bpp = fb_info[2];
	int xoffset = fb_info[3]; 
	int yoffset = fb_info[4];
	int fb_line_length = fb_info[5];
	long int screensize = width*height*bpp;		// = 0x7e9000

	// Check if the mailbox is shielded
	ret = core_mmu_add_mapping(MEM_AREA_IO_NSEC, s2_4kb_table_info_phys, 0x00001000);
	if(ret!=0x1){
		return TEE_ERROR_BAD_STATE;
	}
	s2_4kb_table_info_virt = phys_to_virt (s2_4kb_table_info_phys, MEM_AREA_IO_NSEC);
	for(int i=0; i<10; i++){
		if(*s2_4kb_table_info_virt==MYTEE_MMIO_CONTEXT_DEVICE_FRAMEBUFFER)
			break;
		s2_4kb_table_info_virt++;		// move 8byte
		s2_4kb_table_info_virt++;
		if(i==9)
			return TEE_ERROR_BAD_STATE;		
	}
	// Check if the trusted flag is set
	const unsigned long trusted_display_mmap_flag_phys = MYTEE_TRUSTED_FB_MMAP_FLAG_PHYS;
	uint32_t* trusted_flag_virt;	

	ret = core_mmu_add_mapping(MEM_AREA_IO_NSEC, trusted_display_mmap_flag_phys, 0x00001000);
	if(ret!=0x1){
		return TEE_ERROR_BAD_STATE;
	}
	trusted_flag_virt = phys_to_virt (trusted_display_mmap_flag_phys, MEM_AREA_IO_NSEC);
	if(*trusted_flag_virt!=1)
		return TEE_ERROR_BAD_STATE;
		
	// Check the log
	ret = core_mmu_add_mapping(MEM_AREA_IO_NSEC, setting_count_phys, 0x00001000);
	if(ret!=0x1){
		return TEE_ERROR_BAD_STATE;
	}

	setting_count_virt = phys_to_virt (setting_count_phys, MEM_AREA_IO_NSEC);

	uint32_t* tmp = setting_count_virt;
	context = ++tmp;
	log_page_virtaddr = ++tmp;				// setting_count_virt + 0x8
	
	// To handle multi core... TA will try to synchronize with the ioctl handler of mbox
	while(100){
		if(*context != (FB_LOG_CONTEXT_MMAP | CTX_MBOX_LOG_COMPLETE)){
			if(wait_count_mmap==99){
				IMSG("verify fail.. Context is not equals to logged context");	//TODO: out of sync because of cache
				wait_count_mmap=0;
				*trusted_flag_virt = 0;			// Trusted display flag is cleared
				*s2_4kb_table_info_virt += 0x10;		// Add the verification success flag
				memset(setting_count_virt, 0x0, 0x1000);	// clear setting count and log page
				return TEE_ERROR_BAD_STATE;
			}
			wait_count_mmap++;
			tee_time_wait(100);	// Sync with mbox log and TA
						// wait for mbox log
		}
		else if (*context == (FB_LOG_CONTEXT_MMAP | CTX_MBOX_LOG_COMPLETE)){
			break;
		}
	}
	unsigned int msg_size = *log_page_virtaddr;
	unsigned long* log_ptr = log_page_virtaddr;
	
	// Debugging purpose
	unsigned long* log_ptr_test = log_page_virtaddr;
	for(int i=0; i<msg_size/4; i++){
		IMSG("msg log[%d] : %lx\n", i, *log_ptr_test);
		log_ptr_test++;
	}
	// Debugging purpose

	// Verify the mbox msg log against the TA log
	// Verify based on the struct fb_alloc_tags (bcm2708_fb.c)
	log_ptr+=0x2;	// log[0] = msg size, log[1] = request(0x0)	
	if(*log_ptr!=RPI_FIRMWARE_FRAMEBUFFER_SET_PHYSICAL_WIDTH_HEIGHT)
		return TEE_ERROR_BAD_STATE;
	log_ptr+=0x3;
	if(*log_ptr!=fb_vscreeninfo_log_fb_mmap.xres)
		return TEE_ERROR_BAD_STATE;
	log_ptr+=0x1;
	if(*log_ptr!=fb_vscreeninfo_log_fb_mmap.yres)
		return TEE_ERROR_BAD_STATE;
	log_ptr+=0x1;
	if(*log_ptr!=RPI_FIRMWARE_FRAMEBUFFER_SET_VIRTUAL_WIDTH_HEIGHT)
		return TEE_ERROR_BAD_STATE;
	log_ptr+=0x3;
	if(*log_ptr!=fb_vscreeninfo_log_fb_mmap.xres_virtual)
		return TEE_ERROR_BAD_STATE;
	log_ptr+=0x1;
	if(*log_ptr!=fb_vscreeninfo_log_fb_mmap.yres_virtual)
		return TEE_ERROR_BAD_STATE;
	log_ptr+=0x1;
	if(*log_ptr!=RPI_FIRMWARE_FRAMEBUFFER_SET_DEPTH)
		return TEE_ERROR_BAD_STATE;
	log_ptr+=0x3;
	if(*log_ptr!=fb_vscreeninfo_log_fb_mmap.bits_per_pixel)
		return TEE_ERROR_BAD_STATE;
	log_ptr+=0x1;
	if(*log_ptr!=RPI_FIRMWARE_FRAMEBUFFER_SET_VIRTUAL_OFFSET)
		return TEE_ERROR_BAD_STATE;
	log_ptr+=0x3;
	if(*log_ptr!=fb_vscreeninfo_log_fb_mmap.xoffset)
		return TEE_ERROR_BAD_STATE;
	log_ptr+=0x1;
	if(*log_ptr!=fb_vscreeninfo_log_fb_mmap.yoffset)
		return TEE_ERROR_BAD_STATE;

	// Get phys addr of fb
	ret = core_mmu_add_mapping(MEM_AREA_IO_NSEC, mmapLog_phys, 0x00001000);
	if(ret!=0x1){
		return TEE_ERROR_BAD_STATE;
	}
	mmapLog_virt = phys_to_virt (mmapLog_phys, MEM_AREA_IO_NSEC);
		
	unsigned long framebuffer_phys = mmapLog_virt[0];
	int max_size = mmapLog_virt[1];
	
	//IMSG("memory mapping physaddr of fb");
	//IMSG("frame buffer addr : %x, max_size : %x", framebuffer_phys, max_size);
	ret = core_mmu_add_mapping(MEM_AREA_IO_NSEC, framebuffer_phys, 0x00800000);
	if(ret!=0x1){		
		DMSG("mapping error, please retry after reboot");
		*trusted_flag_virt = 0;
		return TEE_ERROR_BAD_STATE;
	}
	framebuffer_virt = phys_to_virt (framebuffer_phys, MEM_AREA_IO_NSEC);

	int size = 4096 * 400;

	// Use the framebuffer
        for(int i=0 ; i < size;i++){
                framebuffer_virt[i] = i;
        }
        
	// Exit, do init data
	*trusted_flag_virt = 0;			
	*s2_4kb_table_info_virt += 0x10;		// Add verification success flag
	memset(setting_count_virt, 0x0, 0x1000);	// Clear flag and log

	return TEE_SUCCESS;
}

static TEE_Result invoke_command(void *psess __unused,
				 uint32_t cmd, uint32_t ptypes,
				 TEE_Param params[TEE_NUM_PARAMS])
{
	switch (cmd) {
	case TA_MYTA_FB_MMAP_INIT:
		return set_up_trusted_display_flag_on(ptypes, params);
	case TA_MYTA_FB_MMAP_TRUSTED_DISPLAY_START:
		return mmap_ta_and_trusted_display_start(ptypes, params);
	default:
		break;
	}
	return TEE_ERROR_BAD_PARAMETERS;
}
pseudo_ta_register(.uuid = TA_MYTA_FB_MMAP_UUID, .name = "myta_fb_mmap.pta",
		   .flags = PTA_DEFAULT_FLAGS,
		   .invoke_command_entry_point = invoke_command);
