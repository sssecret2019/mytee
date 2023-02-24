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
#include <myta_fb_write.h>
#include <kernel/tee_time.h>
unsigned int wait_count_fb_write=0;

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
} fb_vscreeninfo_log_fb_write;

static TEE_Result set_up_trusted_display_flag_on(uint32_t param_types,
	TEE_Param params[4])
{
	struct fb_var_screeninfo* framebuffer_variable_screeninfo = (struct fb_var_screeninfo*)params[0].memref.buffer;
	const unsigned long trusted_display_write_flag_phys = 0x0f10b008;
	uint32_t* trusted_flag_virt;
	bool ret;	
	
	// set value of framebuffer and loging value
	fb_vscreeninfo_log_fb_write.xres = framebuffer_variable_screeninfo->xres=0x780;
	fb_vscreeninfo_log_fb_write.yres = framebuffer_variable_screeninfo->yres=0x438;
	fb_vscreeninfo_log_fb_write.xres_virtual = framebuffer_variable_screeninfo->xres_virtual=0x780;
	fb_vscreeninfo_log_fb_write.yres_virtual = framebuffer_variable_screeninfo->yres_virtual=0x438;
	fb_vscreeninfo_log_fb_write.bits_per_pixel = framebuffer_variable_screeninfo->bits_per_pixel=0x20;
	fb_vscreeninfo_log_fb_write.xoffset = framebuffer_variable_screeninfo->xoffset=0x0;
	fb_vscreeninfo_log_fb_write.yoffset = framebuffer_variable_screeninfo->yoffset=0x0;

	IMSG("trusted display flag on");
	ret = core_mmu_add_mapping(MEM_AREA_IO_NSEC, trusted_display_write_flag_phys, 0x00001000);
	if(ret!=0x1){
		return TEE_ERROR_BAD_STATE;
	}
	trusted_flag_virt = phys_to_virt (trusted_display_write_flag_phys, MEM_AREA_IO_NSEC);
	*trusted_flag_virt = 1;
	
	return TEE_SUCCESS;
}

static TEE_Result write_to_secure_buffer(uint32_t param_types,
	TEE_Param params[4])
{
	int* write_size = (int*)params[0].memref.buffer;
	const unsigned long trusted_display_write_flag_phys = MYTEE_TRUSTED_FB_WRITE_FLAG_PHYS;
	const unsigned long writeLog_phys = FBWRITELOG;
	const unsigned long sec_src_phys = MYTEE_TRUSTED_FB_WRITE_SECURE_SRC_PHYS;
	const unsigned long setting_count_phys = MYTEE_TRUSTED_FB_MAIL_BOX_SETTING_COUNT_PHYS;
	const unsigned long s2_4kb_table_info_phys = MYTEE_HYP_DYNAMICALLY_ALLOCATED_STAGE2_PAGE_TABLE_4KB_INFORM_PHYS;
	unsigned long* s2_4kb_table_info_virt;
	unsigned long* log_page_virtaddr;
	uint32_t* setting_count_virt;
	unsigned long* writeLog_virt;
	char* sec_src_virt;
	uint32_t* trusted_flag_virt;
	uint32_t* context;
	bool ret;	
	
	// check if the mailbox is shielded
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

	//check if trusted display is setted
	ret = core_mmu_add_mapping(MEM_AREA_IO_NSEC, trusted_display_write_flag_phys, 0x00001000);
	if(ret!=0x1){
		return TEE_ERROR_BAD_STATE;
	}
	trusted_flag_virt = phys_to_virt (trusted_display_write_flag_phys, MEM_AREA_IO_NSEC);
	if(*trusted_flag_virt!=1)
		return TEE_ERROR_BAD_STATE;
		
	// check log
	ret = core_mmu_add_mapping(MEM_AREA_IO_NSEC, setting_count_phys, 0x00001000);
	if(ret!=0x1){
		return TEE_ERROR_BAD_STATE;
	}

	setting_count_virt = phys_to_virt (setting_count_phys, MEM_AREA_IO_NSEC);
	uint32_t* tmp = setting_count_virt;
	context = ++tmp;
	log_page_virtaddr = ++tmp;				// setting_count_virt + 0x8
	
	// Multi core synchonization 
	while(100){
		if(*context != (FB_LOG_CONTEXT_WRITE | CTX_MBOX_LOG_COMPLETE)){
			if(wait_count_fb_write==99){
				wait_count_fb_write=0;
				IMSG("verify fail.. Context is not equals to logged context");
				*trusted_flag_virt = 0;			
				*s2_4kb_table_info_virt += 0x10;		// Add verification success flag
				memset(setting_count_virt, 0x0, 0x1000);	// Clear count and log 
				return TEE_ERROR_BAD_STATE;
			}
			wait_count_fb_write++;
			tee_time_wait(100);	// Sync with mbox log and TA
						// wait 0.02 second for mbox log
		}
		else if (*context == (FB_LOG_CONTEXT_WRITE | CTX_MBOX_LOG_COMPLETE)){
			break;
		}
	}
	unsigned int msg_size = *log_page_virtaddr;
	unsigned long* log_ptr = log_page_virtaddr;
	
	// For debugging
	unsigned long* log_ptr_test = log_page_virtaddr;
	for(int i=0; i<msg_size/4; i++){
		IMSG("msg log[%d] : %lx\n", i, *log_ptr_test);
		log_ptr_test++;
	}
	// End debug

	// Verify the mbox msg log against the TA log
	// Verify based on struct fb_alloc_tags (bcm2708_fb.c)
	log_ptr+=0x2;	// log[0] = msg size, log[1] = request(0x0)	
	if(*log_ptr!=RPI_FIRMWARE_FRAMEBUFFER_SET_PHYSICAL_WIDTH_HEIGHT)
		return TEE_ERROR_BAD_STATE;
	log_ptr+=0x3;
	if(*log_ptr!=fb_vscreeninfo_log_fb_write.xres)
		return TEE_ERROR_BAD_STATE;
	log_ptr+=0x1;
	if(*log_ptr!=fb_vscreeninfo_log_fb_write.yres)
		return TEE_ERROR_BAD_STATE;
	log_ptr+=0x1;
	if(*log_ptr!=RPI_FIRMWARE_FRAMEBUFFER_SET_VIRTUAL_WIDTH_HEIGHT)
		return TEE_ERROR_BAD_STATE;
	log_ptr+=0x3;
	if(*log_ptr!=fb_vscreeninfo_log_fb_write.xres_virtual)
		return TEE_ERROR_BAD_STATE;
	log_ptr+=0x1;
	if(*log_ptr!=fb_vscreeninfo_log_fb_write.yres_virtual)
		return TEE_ERROR_BAD_STATE;
	log_ptr+=0x1;
	if(*log_ptr!=RPI_FIRMWARE_FRAMEBUFFER_SET_DEPTH)
		return TEE_ERROR_BAD_STATE;
	log_ptr+=0x3;
	if(*log_ptr!=fb_vscreeninfo_log_fb_write.bits_per_pixel)
		return TEE_ERROR_BAD_STATE;
	log_ptr+=0x1;
	if(*log_ptr!=RPI_FIRMWARE_FRAMEBUFFER_SET_VIRTUAL_OFFSET)
		return TEE_ERROR_BAD_STATE;
	log_ptr+=0x3;
	if(*log_ptr!=fb_vscreeninfo_log_fb_write.xoffset)
		return TEE_ERROR_BAD_STATE;
	log_ptr+=0x1;
	if(*log_ptr!=fb_vscreeninfo_log_fb_write.yoffset)
		return TEE_ERROR_BAD_STATE;
	
	// Start writing to the secure buffer
	*write_size = 4096 * 400;
	
	// Log the write_size
	ret = core_mmu_add_mapping(MEM_AREA_IO_NSEC, writeLog_phys, 0x00001000);
	if(ret!=0x1){
		return TEE_ERROR_BAD_STATE;
	}
	writeLog_virt = phys_to_virt (writeLog_phys, MEM_AREA_IO_NSEC);
	writeLog_virt[1] = *write_size;
	
	ret = core_mmu_add_mapping(MEM_AREA_IO_NSEC, sec_src_phys, 0x00800000);
	if(ret!=0x1){
		IMSG("mapping error, please retry after reboot");
		*trusted_flag_virt = 0;
		return TEE_ERROR_BAD_STATE;
	}
	sec_src_virt = phys_to_virt (sec_src_phys, MEM_AREA_IO_NSEC);
	
	IMSG("write to secure buffer");
        for(int i=0 ; i <*write_size;i++)
                sec_src_virt[i] = i;
	
	return TEE_SUCCESS;
}

static TEE_Result set_up_trusted_display_flag_off(uint32_t param_types,
	TEE_Param params[4])
{
	const unsigned long trusted_display_write_flag_phys = MYTEE_TRUSTED_FB_WRITE_FLAG_PHYS;
	const unsigned long writeLog_phys = FBWRITELOG;
	unsigned long* writeLog_virt;
	uint32_t* trusted_flag_virt;
	const unsigned long setting_count_phys = MYTEE_TRUSTED_FB_MAIL_BOX_SETTING_COUNT_PHYS;
	const unsigned long s2_4kb_table_info_phys = MYTEE_HYP_DYNAMICALLY_ALLOCATED_STAGE2_PAGE_TABLE_4KB_INFORM_PHYS;
	unsigned long* s2_4kb_table_info_virt;
	unsigned long* log_page_virtaddr;
	uint32_t* setting_count_virt;
	bool ret;	

	ret = core_mmu_add_mapping(MEM_AREA_IO_NSEC, writeLog_phys, 0x00001000);
	if(ret!=0x1){
		return TEE_ERROR_BAD_STATE;
	}
	writeLog_virt = phys_to_virt (writeLog_phys, MEM_AREA_IO_NSEC);
		
	// Check the written address
	unsigned long address = writeLog_virt[0];
	IMSG("check address : 0x%x", address);
	if(address==0x0){
		IMSG("address translation aborted");
		return TEE_ERROR_BAD_STATE;
	}
	else if(address!=FRAMEBUFFER_PHYS)
		return TEE_ERROR_BAD_STATE;
	
	// exit, do init data	
	ret = core_mmu_add_mapping(MEM_AREA_IO_NSEC, trusted_display_write_flag_phys, 0x00001000);
	if(ret!=0x1){
		return TEE_ERROR_BAD_STATE;
	}
	trusted_flag_virt = phys_to_virt (trusted_display_write_flag_phys, MEM_AREA_IO_NSEC);
	
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
	
	ret = core_mmu_add_mapping(MEM_AREA_IO_NSEC, setting_count_phys, 0x00001000);
	if(ret!=0x1){
		return TEE_ERROR_BAD_STATE;
	}

	setting_count_virt = phys_to_virt (setting_count_phys, MEM_AREA_IO_NSEC);
	
	*trusted_flag_virt = 0;			
	*s2_4kb_table_info_virt += 0x10;		// Add verification success flag
	memset(setting_count_virt, 0x0, 0x1000);	// clear setting count and log page
	
	return TEE_SUCCESS;
}

static TEE_Result invoke_command(void *psess __unused,
				 uint32_t cmd, uint32_t ptypes,
				 TEE_Param params[TEE_NUM_PARAMS])
{
	switch (cmd) {
	case TA_MYTA_FB_WRITE_INIT:
		return set_up_trusted_display_flag_on(ptypes, params);
	case TA_MYTA_FB_WRITE_SECURE_BUFFER:
		return write_to_secure_buffer(ptypes, params);
	case TA_MYTA_FB_WRITE_TRUSTED_DISPLAY_END:
		return set_up_trusted_display_flag_off(ptypes, params);	
	default:
		break;
	}
	return TEE_ERROR_BAD_PARAMETERS;
}
pseudo_ta_register(.uuid = TA_MYTA_FB_WRITE_UUID, .name = "myta_fb_write.pta",
		   .flags = PTA_DEFAULT_FLAGS,
		   .invoke_command_entry_point = invoke_command);
