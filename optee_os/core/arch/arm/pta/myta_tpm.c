#include <tee_internal_api.h>
#include <tee_internal_api_extensions.h>
#include <kernel/pseudo_ta.h>
#include <string.h>
#include <mm/core_mmu.h>
#include <mm/core_memprot.h>
#include <myta_tpm.h>
#include <eltt2.h>

uint8_t command_log[4096];
int command_size_log;

static TEE_Result create_tpm_command(uint32_t param_types,
	TEE_Param params[4])
{
	IMSG("TA call!!!!!!!\n");
	unsigned int* request = (unsigned int*)params[0].memref.buffer;
	char* optarg = (char *)params[1].memref.buffer;
	uint8_t* command = (uint8_t*)params[2].memref.buffer;
	unsigned int* command_size = (unsigned int*)params[3].memref.buffer;
	hash_algo_enum hash_algo = ALG_NULL;
	
	IMSG("request : %d\n", *request);
	IMSG("optarg : %s\n", optarg);
	
	const unsigned long trusted_flag_phys = MYTEE_TRUSTED_TPM_FLAG_PHYS;
	uint32_t* trusted_flag_virt;	

	core_mmu_add_mapping(MEM_AREA_IO_NSEC, trusted_flag_phys, 0x00001000);
	trusted_flag_virt = phys_to_virt (trusted_flag_phys, MEM_AREA_IO_NSEC);
	*trusted_flag_virt = 1;
	
	switch (*request)
	{
		case GET_RANDOM:
			*command_size = (sizeof(tpm2_getrandom));
			memset(command, 0, *command_size);
			get_random(optarg, command);
			break;
		case SHA1:
			hash_algo = ALG_SHA1;

			// Allocate the input buffer for create_hash and tpmtool_transmit.
			*command_size = strlen(optarg) / HEX_BYTE_STRING_LENGTH + strlen(optarg) % HEX_BYTE_STRING_LENGTH + sizeof(tpm2_hash);
			if(*command_size>COMMAND_MAX_SIZE){
				IMSG("TPM Command size exceeded. Please enter a smaller data size.");
				break;
			}
			memset(command, 0, *command_size);

			// Create Hash TPM request.
			create_hash(optarg, hash_algo, command, *command_size);

			break;
		case SHA256:
			hash_algo = ALG_SHA256;

			// Allocate the input buffer for create_hash and tpmtool_transmit.
			*command_size = strlen(optarg) / HEX_BYTE_STRING_LENGTH + strlen(optarg) % HEX_BYTE_STRING_LENGTH + sizeof(tpm2_hash);
			if(*command_size>COMMAND_MAX_SIZE){
				IMSG("Command size exceeded. Please enter a smaller data size.");
				break;
			}
			memset(command, 0, *command_size);

			// Create Hash TPM request.
			create_hash(optarg, hash_algo, command, *command_size);
			
			break;
		case STARTUP_CLEAR:
			*command_size = (sizeof(tpm2_startup_clear));
			memcpy(command, tpm2_startup_clear, sizeof(tpm2_startup_clear));
			break;
		case STARTUP_STATE:
			*command_size = (sizeof(tpm2_startup_state));		
			memcpy(command, tpm2_startup_state, sizeof(tpm2_startup_state));
			break;
	}

	memcpy(command_log, command, *command_size);
	command_size_log = *command_size;

	return TEE_SUCCESS;
}

static TEE_Result verify_log_and_use_tpmValue(uint32_t param_types,
	TEE_Param params[4])
{
	unsigned int* request = (unsigned int*)params[0].memref.buffer;
	ssize_t* tpm_response_buf_size = (ssize_t*)params[1].memref.buffer;
	uint8_t* tpm_value_test_buf = (uint8_t*)params[2].memref.buffer;
	uint8_t* command_test_buf = (uint8_t*)params[3].memref.buffer;
	int ret_val;
	const unsigned long secure_rx_buf_phys = MYTEE_HYP_SECURE_SPI_TPM_RX_BUF_PHYS;
	const unsigned long s2_4kb_table_info_phys = MYTEE_HYP_DYNAMICALLY_ALLOCATED_STAGE2_PAGE_TABLE_4KB_INFORM_PHYS;
	const unsigned long p_log_idx_phys = MYTEE_HYP_SECURE_TPM_LOG_PAGE_PHYS;
	uint8_t* log_page_virtaddr;
	uint32_t* p_log_idx;
	uint32_t* p_log_last_ctx_idx;
	unsigned long long* s2_4kb_table_info_virtaddr; 
	void* secure_buf_virtaddr;
	uint8_t context;	
	

	// Check the log
	core_mmu_add_mapping(MEM_AREA_IO_NSEC, p_log_idx_phys, 0x00001000);
	p_log_idx = phys_to_virt (p_log_idx_phys, MEM_AREA_IO_NSEC);
	
	uint32_t* tmp = p_log_idx;
	p_log_last_ctx_idx = ++tmp;		// offset p_log_idx + 0x4
	log_page_virtaddr = ++tmp;				// offset p_log_idx + 0x8
	log_page_virtaddr += *p_log_last_ctx_idx;

	context = *log_page_virtaddr;
	if(context!=SPI_FIFO_WRITE)
		return TEE_ERROR_BAD_STATE;
		
	log_page_virtaddr++;
	
	uint8_t* command_log_ptr = command_log;
	for(int i=0; i<command_size_log; i++){
		if(*command_log_ptr!=*log_page_virtaddr)
			return TEE_ERROR_BAD_STATE;
		command_log_ptr++;
		log_page_virtaddr++;
	}
	
	// If log is ok, TA can use TPM value.
	core_mmu_add_mapping(MEM_AREA_IO_NSEC, secure_rx_buf_phys, 0x00001000);
	secure_buf_virtaddr = phys_to_virt (secure_rx_buf_phys, MEM_AREA_IO_NSEC);
	
	response_print(secure_buf_virtaddr, *tpm_response_buf_size, *request);
		
	// For unshielding, TA will log the consumption of TPM output. It will be checked in monitor.
	core_mmu_add_mapping(MEM_AREA_IO_NSEC, s2_4kb_table_info_phys, 0x00001000);
	s2_4kb_table_info_virtaddr = phys_to_virt (s2_4kb_table_info_phys, MEM_AREA_IO_NSEC);

	for(int i = 0; i < 10; i++){		
		if(*s2_4kb_table_info_virtaddr == MYTEE_MMIO_CONTEXT_DEVICE_TPM){ 
			*s2_4kb_table_info_virtaddr += 0x10; // Add the verification success flag
			break;
		}
		s2_4kb_table_info_virtaddr++;
		if(i==10)
			return TEE_ERROR_BAD_STATE;
	}
	
	const unsigned long trusted_flag_phys = MYTEE_TRUSTED_TPM_FLAG_PHYS;
	uint32_t* trusted_flag_virt;

	core_mmu_add_mapping(MEM_AREA_IO_NSEC, trusted_flag_phys, 0x00001000);
	trusted_flag_virt = phys_to_virt (trusted_flag_phys, MEM_AREA_IO_NSEC);
	*trusted_flag_virt = 0;
	uint32_t* request_Count = ++trusted_flag_virt;
	*request_Count = 0;
	
	// For testing
	// The TPM result should not be exposed to the REE, but here, we send it to the CA for testing.
	memcpy(tpm_value_test_buf, secure_buf_virtaddr, *tpm_response_buf_size);
	// command log test
	memcpy(command_test_buf, p_log_idx, 100);
		
	return TEE_SUCCESS;
}

// The code from the Infenion TPM application.
// Maybe, can be further slimmed down.

static int hexstr_to_bytearray(char *byte_string, uint8_t *byte_values, size_t byte_values_size)
{
	int ret_val = EXIT_SUCCESS;	// Return value.
	char hex_byte[3] = {0};		// Temporary buffer for input bytes.
	char* invalidChars = NULL;	// Pointer to target buffer where method stores invalid characters.
	uint32_t i = 0;			// Loop variable.
	uint32_t unStrLen = 0;		// Temporary store for byte string length.

	do
	{
		NULL_POINTER_CHECK(byte_string);
		NULL_POINTER_CHECK(byte_values);

		if (0 >= byte_values_size)
		{
			ret_val = EINVAL;
			IMSG("Bad parameter. Value of parameter 'byte_values_size' must be larger than 0.\n");
		break;
		}
		if (INT_MAX < byte_values_size)
		{
			ret_val = EINVAL;
			IMSG("Bad parameter. Value of parameter 'byte_values_size' must be smaller or equal to %u.\n", INT_MAX);
			break;
		}

		memset(byte_values, 0, byte_values_size);

		unStrLen = strlen(byte_string);
		if ((unStrLen / HEX_BYTE_STRING_LENGTH + unStrLen % HEX_BYTE_STRING_LENGTH) > (uint32_t)byte_values_size)
		{
			ret_val = EINVAL;
			IMSG("Bad parameter. Input character string is too long for output buffer.\n");
			break;
		}

		// Loop "byte-wise" through string
		for (i = 0; i < unStrLen; i += HEX_BYTE_STRING_LENGTH)
		{
			// Split input string into "bytes"
			if (1 == strlen(byte_string + i))
			{
				// Assemble a single digit in the hex byte string.
				hex_byte[0] = byte_string[i];
				hex_byte[1] = '\0';
				hex_byte[2] = '\0';
			}
			else
			{
				// Assemble a digit pair in the hex byte string.
				hex_byte[0] = byte_string[i];
				hex_byte[1] = byte_string[i + 1];
				hex_byte[2] = '\0';
			}

			// Convert the hex string to an integer.
			byte_values[i / HEX_BYTE_STRING_LENGTH] = (uint8_t)strtoul(hex_byte, &invalidChars, 16);
			if ('\0' != *invalidChars)
			{
				ret_val = EINVAL;
				IMSG("Invalid character(s) '%s' while trying to parse '%s' to a byte array.\n", invalidChars, byte_string);
				break;
			}
		}
	} while (0);

	return ret_val;
}

static int int_to_bytearray(uint64_t input, uint32_t input_size, uint8_t *output_byte)
{
	int ret_val = EXIT_SUCCESS;	// Return value.
	uint32_t i;			// For-while-loop counter.

	do
	{
		NULL_POINTER_CHECK(output_byte);
		if (0 >= input_size)
		{
			ret_val = EINVAL;
			IMSG("Bad parameter. Value of parameter 'input_size' must be larger than 0.\n");
			break;
		}
		if (sizeof(uint64_t) < input_size)
		{
			ret_val = EINVAL;
			IMSG("Bad parameter. Value of parameter 'input_size' must be smaller or equal to %zu.\n", sizeof(uint64_t));
			break;
		}

		for (i = 0; i < input_size; i++)
		{
			output_byte[i] = input >> ((input_size - 1 - i) * 8);
		}
	} while (0);

	return ret_val;
}

static int get_random(char *data_length_string, uint8_t *response_buf)
{
	int ret_val = EXIT_SUCCESS;	// Return value.
	uint8_t bytes_requested = 0;	// Amount of random bytes requested by the user.
	size_t byte_string_size = 0;	// Size of user input.

	do
	{
		NULL_POINTER_CHECK(data_length_string);
		NULL_POINTER_CHECK(response_buf);

		// Get length of command line input.
		byte_string_size = strlen(data_length_string) / HEX_BYTE_STRING_LENGTH + strlen(data_length_string) % HEX_BYTE_STRING_LENGTH;
		if (1 != byte_string_size)
		{
			ret_val = ERR_BAD_CMD;
			IMSG("Bad option. Enter a single hex value (2 characters) without leading '0x'.\n");
			break;
		}

		// Convert the command line input string for requested random data length to byte.
		ret_val = hexstr_to_bytearray(data_length_string, &bytes_requested, 1);
		RET_VAL_CHECK(ret_val);
		if (32 < bytes_requested || 0 == bytes_requested)
		{
			ret_val = ERR_BAD_CMD;
			IMSG("Bad option. Enter a hex value between 0x01 and 0x20.\n");
			break;
		}

		// Copy command bytes.
		memcpy(response_buf, tpm2_getrandom, sizeof(tpm2_getrandom));

		// Store amount of requested bytes at the correct byte index in the command byte stream.
		response_buf[sizeof(tpm2_getrandom) - 1] = bytes_requested;
	} while (0);

	return ret_val;
}

static int create_hash(char *data_string, hash_algo_enum hash_algo, uint8_t *hash_cmd_buf, uint32_t hash_cmd_buf_size)
{
	int ret_val = EXIT_SUCCESS;		// Return value.
	uint32_t offset = 0;			// Helper offset for generating command request.
	uint16_t data_string_size = 0;		// Size of user input data.
	const uint8_t *tpm_hash_alg = NULL;	// Pointer to hash algorithm identifier.

	do
	{
		NULL_POINTER_CHECK(data_string);
		NULL_POINTER_CHECK(hash_cmd_buf);

		if (TPM_REQ_MAX_SIZE < hash_cmd_buf_size)
		{
			ret_val = EINVAL;
			IMSG("Bad parameter. Value of parameter 'hash_cmd_buf_size' must be smaller or equal to %u.\n", TPM_REQ_MAX_SIZE);
			break;
		}
		if (sizeof(tpm2_hash) > hash_cmd_buf_size)
		{
			ret_val = EINVAL;
			IMSG("Bad parameter. Value of parameter 'hash_cmd_buf_size' must be at least %zu.\n", sizeof(tpm2_hash));
			break;
		}
		data_string_size = strlen(data_string) / HEX_BYTE_STRING_LENGTH + strlen(data_string) % HEX_BYTE_STRING_LENGTH;
		if (0 == data_string_size)
		{
			ret_val = EINVAL;
			IMSG("Bad parameter. data_string is empty.\n");
			break;
		}
		if (hash_cmd_buf_size - sizeof(tpm2_hash) < data_string_size)
		{
			ret_val = EINVAL;
			IMSG("Bad parameter. Input data size must be smaller or equal to %zu.\n", hash_cmd_buf_size - sizeof(tpm2_hash));
			break;
		}

		memset(hash_cmd_buf, 0, hash_cmd_buf_size);

		// Copy basic command bytes.
		memcpy(hash_cmd_buf, tpm2_hash, sizeof(tpm2_hash));

		// Set hash algorithm, command and data sizes depending on user input option at the correct byte index in the command byte stream.
		if (ALG_SHA1 == hash_algo)
		{
			tpm_hash_alg = sha1_alg;
			IMSG("\nTPM2_Hash of '%s' with SHA-1:\n", data_string);
		}
		else if (ALG_SHA256 == hash_algo)
		{
			tpm_hash_alg = sha256_alg;
			IMSG("\nTPM2_Hash of '%s' with SHA-256:\n", data_string);
		}
		else
		{
			tpm_hash_alg = sha384_alg;
			IMSG("\nTPM2_Hash of '%s' with SHA-384:\n", data_string);
		}

		offset = TPM_CMD_SIZE_OFFSET;
		ret_val = int_to_bytearray(sizeof(tpm2_hash) + data_string_size, sizeof(uint32_t), hash_cmd_buf + offset);
		RET_VAL_CHECK(ret_val);
		offset = TPM_CMD_HEADER_SIZE;
		ret_val = int_to_bytearray(data_string_size, sizeof(data_string_size), hash_cmd_buf + offset);
		RET_VAL_CHECK(ret_val);
		offset += sizeof(data_string_size);

		// Copy hash data to TPM request.
		ret_val = hexstr_to_bytearray(data_string, hash_cmd_buf + offset, hash_cmd_buf_size - offset);
		RET_VAL_CHECK(ret_val);
		offset += data_string_size;

		// Set hash algorithm and hierarchy.
		memcpy(hash_cmd_buf + offset, tpm_hash_alg, sizeof(sha1_alg));
		offset += sizeof(sha1_alg);
		memcpy(hash_cmd_buf + offset, tpm_cc_hash_hierarchy, sizeof(tpm_cc_hash_hierarchy));
	} while (0);

	return ret_val;
}

static int response_print(uint8_t *response_buf, size_t resp_size, unsigned int request)
{
	int ret_val = EXIT_SUCCESS;	// Return value.

	do
	{
		NULL_POINTER_CHECK(response_buf);

		if (0 >= resp_size)
		{
			ret_val = EINVAL;
			IMSG("Bad parameter. Value of parameter 'resp_size' must be larger than 0.");
			break;
		}
		if (TPM_RESP_MAX_SIZE < resp_size)
		{
			ret_val = EINVAL;
			IMSG("Bad parameter. Value of parameter 'resp_size' must be smaller than or equal to %u.\n", TPM_RESP_MAX_SIZE);
			break;
		}

		switch (request)
		{
			case GET_RANDOM:
				IMSG("Random value:\n");
				ret_val = print_response_buf(response_buf, resp_size, PRINT_RESPONSE_WITHOUT_HEADER, PRINT_RESPONSE_HEX_BLOCK);
				break;
			case SHA1:
				ret_val = print_response_buf(response_buf, resp_size, PRINT_RESPONSE_WITHOUT_HEADER, PRINT_RESPONSE_HASH);
				break;
			case SHA256:
				ret_val = print_response_buf(response_buf, resp_size, PRINT_RESPONSE_WITHOUT_HEADER, PRINT_RESPONSE_HASH);
			
				break;
			case STARTUP_CLEAR:
				IMSG("Startup works as expected.\n");
				break;
			case STARTUP_STATE:
				IMSG("Startup works as expected.\n");
				break;
		}
		IMSG("\n");
	} while (0);

	return ret_val;
}

static int print_response_buf(uint8_t *response_buf, size_t resp_size, uint32_t offset, int format)
{
	int ret_val = EXIT_SUCCESS;	// Return value.
	uint32_t i = 0;			// Loop variable.
	uint64_t data_size = 0;		// Size of response data.

	do
	{
		NULL_POINTER_CHECK(response_buf);

		if (0 >= resp_size)
		{
			ret_val = EINVAL;
			IMSG("Bad parameter. Value of parameter 'resp_size' must be larger than 0.");
			break;
		}
		if (TPM_RESP_MAX_SIZE < resp_size)
		{
			ret_val = EINVAL;
			IMSG("Bad parameter. Value of parameter 'resp_size' must be smaller than or equal to %u.\n", TPM_RESP_MAX_SIZE);
			break;
		}
		if (resp_size <= offset)
		{
			ret_val = EINVAL;
			IMSG("Bad parameter. Offset %u cannot be equal or larger than input buffer size %zu.\n", offset, resp_size);
			break;
		}

		switch (format)
		{
			case PRINT_RESPONSE_CLEAR:
				for (i = 0; i < resp_size - offset; i++)
				{
					IMSG("%02X ", response_buf[i + offset]);
				}
				break;

			case PRINT_RESPONSE_HEADERBLOCKS:
				if (TPM_CMD_HEADER_SIZE > resp_size)
				{
					ret_val = EINVAL;
					IMSG("Response size is too small.\n");
					break;
				}

				IMSG("TPM Response:\n");
				for (i = 0; i < resp_size - offset; i++)
				{
					IMSG("%02X ", response_buf[i + offset]);
					if (i == 1) // Bytes 0 and 1 are TPM TAG.
					{
						IMSG("                        TPM TAG\n");
					}
					else if (i == 5) // Bytes 2 to 5 are the response length.
					{
						IMSG("                  RESPONSE SIZE\n");
					}
					else if (i == 9) // Last 4 bytes in header are the TPM return code.
					{
						IMSG("                  RETURN CODE\n");
						if (i + 1 < resp_size - offset)
						{
							IMSG(" Command-specific response Data:\n");
						}
					}
					else if (i >= TPM_CMD_HEADER_SIZE && (i+1) % 10 == 0) // After all header bytes have been printed, start new line after every 10 bytes of data.
					{
						IMSG("\n");
					}
				}
				break;

			case PRINT_RESPONSE_HEX_BLOCK:
				for (i = 0; i < resp_size - offset; i++)
				{
					if (i % 16 == 0)
					{
						IMSG("\n0x%08X:   ", i);
					}
					IMSG("0x%02X  ", response_buf[i + offset]);
				}
				break;

			case PRINT_RESPONSE_HASH:
				ret_val = buf_to_uint64(response_buf, offset - sizeof(uint16_t), sizeof(uint16_t), &data_size, resp_size); // Data size actually is only an uint16_t and should always be stored right before the actual data
				if (data_size > resp_size - offset)
				{
					ret_val = EINVAL;
					IMSG("Invalid response data size.\n");
					break;
				}
				for (i = 0; i < data_size; i++)
				{
					if (i % 8 == 0)
					{
						IMSG("\n0x%08X:   ", i);
					}
					IMSG("%02X  ", response_buf[i + offset]);
				}
				break;

			default:
				ret_val = EINVAL;
				IMSG("Unknown output format.\n");
				break;
		}
	} while (0);

	return ret_val;
}

static int buf_to_uint64(uint8_t *input_buffer, uint32_t offset, uint32_t length, uint64_t *output_value, uint32_t input_buffer_size)
{
	int ret_val = EXIT_SUCCESS;	// Return value.
	uint32_t i = 0;			// Loop variable.
	uint64_t tmp = 0;		// Temporary variable for value calculation.

	do
	{
		NULL_POINTER_CHECK(input_buffer);
		NULL_POINTER_CHECK(output_value);

		*output_value = 0;

		if (offset >= input_buffer_size)
		{
			ret_val = EINVAL;
			IMSG("Bad parameter. Value of parameter 'input_buffer_size' must be larger than %u.\n", offset);
			break;
		}
		if (INT_MAX < input_buffer_size)
		{
			ret_val = EINVAL;
			IMSG("Bad parameter. Value of parameter 'input_buffer_size' must be smaller or equal to %u.\n", INT_MAX);
			break;
		}
		if (0 >= length)
		{
			ret_val = EINVAL;
			IMSG("Bad parameter. Value of parameter 'length' must be larger than 0.\n");
			break;
		}
		if (length > input_buffer_size - offset)
		{
			ret_val = EINVAL;
			IMSG("Bad parameter. Value of parameter 'length' must be smaller or equal to %i.\n", input_buffer_size - offset);
			break;
		}
		if (sizeof(uint64_t) < length)
		{
			ret_val = EINVAL;
			IMSG("Bad parameter. Input buffer length remaining from offset must be smaller than %zu.\n", sizeof(uint64_t));
			break;
		}

		for (i = 0; i < length; i++)
		{
			tmp = (tmp << 8) + input_buffer[offset + i];
		}
		*output_value = tmp;
	} while (0);

	return ret_val;
}


static TEE_Result invoke_command(void *psess __unused,
				 uint32_t cmd, uint32_t ptypes,
				 TEE_Param params[TEE_NUM_PARAMS])
{
	switch (cmd) {
	case TA_MYTA_TPM_CREATE_COMMAND:
		return create_tpm_command(ptypes, params);
	case TA_MYTA_TPM_VERIFY_LOG_AND_USE_TPM_VALUE:
		return verify_log_and_use_tpmValue(ptypes, params);
	default:
		break;
	}
	return TEE_ERROR_BAD_PARAMETERS;
}
pseudo_ta_register(.uuid = TA_MYTA_TPM_UUID, .name = "myta_tpm.pta",
		   .flags = PTA_DEFAULT_FLAGS,
		   .invoke_command_entry_point = invoke_command);
