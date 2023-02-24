#include <tee_internal_api.h>
#include <tee_internal_api_extensions.h>
#include <kernel/pseudo_ta.h>
#include <string.h>
#include <mm/core_mmu.h>
#include <mm/core_memprot.h>
#include <myta_keyboard.h>
#include <keyboard_input_keymap.h>

static TEE_Result set_up_trusted_keyboard_flag_on(uint32_t param_types,
	TEE_Param params[4])
{
	const unsigned long trusted_flag_phys = MYTEE_TRUSTED_KEYBOARD_FLAG_PHYS;
	uint32_t* trusted_flag_virt;	

	core_mmu_add_mapping(MEM_AREA_IO_NSEC, trusted_flag_phys, 0x00001000);
	trusted_flag_virt = phys_to_virt (trusted_flag_phys, MEM_AREA_IO_NSEC);
	*trusted_flag_virt = 1;
	
	return TEE_SUCCESS;
}

static TEE_Result set_up_trusted_keyboard_flag_off_and_usb_keyboard_input(uint32_t param_types,
	TEE_Param params[4])
{
	const unsigned long secure_kbd_buffer_phys = MYTEE_HYP_SECURE_KEYBOARD_BUF_PHYS;
	const unsigned long trusted_flag_phys = MYTEE_TRUSTED_KEYBOARD_FLAG_PHYS;
	const unsigned long secure_kbd_buf_index_ta_phys = MYTEE_HYP_SECURE_KEYBOARD_BUF_INDEX_TA_PHYS;
	uint8_t* secure_kbd_buffer_virt;	
	uint32_t* trusted_flag_virt;	
	uint32_t* secure_kbd_buf_index_ta_virt;	
	
	core_mmu_add_mapping(MEM_AREA_IO_NSEC, secure_kbd_buffer_phys, 0x00002000);
	
	secure_kbd_buffer_virt = phys_to_virt (secure_kbd_buffer_phys, MEM_AREA_IO_NSEC);
	trusted_flag_virt = phys_to_virt (trusted_flag_phys, MEM_AREA_IO_NSEC);
	secure_kbd_buf_index_ta_virt = phys_to_virt (secure_kbd_buf_index_ta_phys, MEM_AREA_IO_NSEC);
	
	// Disable trusted kbd flag
	*trusted_flag_virt = 0;
		
	// Get keyboard input from secure buffer
	uint32_t idx = *secure_kbd_buf_index_ta_virt;
	secure_kbd_buffer_virt += idx;
	IMSG("\n");
	
	// For testing.. secret values are sent to the REE
	char* test_buffer_in_REE = (char*)params[0].memref.buffer;
	unsigned int* idx_in_REE = (unsigned int*)params[1].memref.buffer;
	// End testing
	
	while(1){
		if(*secure_kbd_buffer_virt==0x0){
			*secure_kbd_buf_index_ta_virt = idx;	// save buffer index of TA
			break;		
		}
		
		else{
			if(*secure_kbd_buf_index_ta_virt==4096)	// Circular buffer max size
				*secure_kbd_buf_index_ta_virt=0x0;
			if(*secure_kbd_buffer_virt==255)
				IMSG("Code 255 is reserved for special needs of AT keyboard driver");
			else if(*secure_kbd_buffer_virt>0x248)
				IMSG("Code %d is not generic keyboard code (not supported)", *secure_kbd_buffer_virt);
			else
				IMSG("secure key[%d] : %s", idx, ev_key_code_keymap[*secure_kbd_buffer_virt]);
			idx++;
			(*idx_in_REE)++;
			*test_buffer_in_REE = *secure_kbd_buffer_virt;
			secure_kbd_buffer_virt++;
			test_buffer_in_REE++;
		}
	}
	return TEE_SUCCESS;
}

static TEE_Result invoke_command(void *psess __unused,
				 uint32_t cmd, uint32_t ptypes,
				 TEE_Param params[TEE_NUM_PARAMS])
{
	switch (cmd) {
	case TA_MYTA_TRUSTED_KEYBOARD_ON:
		return set_up_trusted_keyboard_flag_on(ptypes, params);
	case TA_MYTA_TRUSTED_KEYBOARD_OFF_AND_USB_KEYBOARD_INPUT:
		return set_up_trusted_keyboard_flag_off_and_usb_keyboard_input(ptypes, params);
	default:
		break;
	}
	return TEE_ERROR_BAD_PARAMETERS;
}
pseudo_ta_register(.uuid = TA_MYTA_KEYBOARD_UUID, .name = "myta_keyboard.pta",
		   .flags = PTA_DEFAULT_FLAGS,
		   .invoke_command_entry_point = invoke_command);
