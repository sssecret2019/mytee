#include <err.h>
#include <stdio.h>
#include <string.h>

#include <tee_client_api.h>

#include <myta_tpm.h>
#include <sys/time.h>
#include <getopt.h>
#include "eltt2.h"
#define MYTEE_IOCTL_REQUEST_UNSHIELD_SPI 0x10

static inline unsigned long long getmicsec(){
	struct timeval tv;
	gettimeofday(&tv, NULL);
	unsigned long long micsec = 1000000*tv.tv_sec+tv.tv_usec;
	return micsec;
}


int main(int argc, char **argv)
{
	unsigned long long start_point, end_point;
//start_point = clock();
	int i = 0;				// Command line parsing counter.
	int option = 0;				// Command line option.
	
	TEEC_Result res;
	TEEC_Context ctx;
	TEEC_Session sess;
	TEEC_Operation op;
	TEEC_UUID uuid = TA_MYTA_TPM_UUID;
	uint32_t err_origin;
	
	uint8_t command[COMMAND_MAX_SIZE];

	unsigned int request = 0;
	unsigned int command_size = 0;
	unsigned int notarg = 0;
	
	int ret_val = EXIT_SUCCESS;
	uint8_t *tpm_response_buf = NULL;	// Buffer for TPM response.
	uint8_t *tpm_value_test_buf = NULL;	// Buffer for TPM response.	// for testing
	ssize_t tpm_response_buf_size = 0;	// Size of tpm_response_buf.
	
	start_point = getmicsec();
	
	// ---------- Program flow ----------

	res = TEEC_InitializeContext(NULL, &ctx);
	if (res != TEEC_SUCCESS)
		errx(1, "TEEC_InitializeContext failed with code 0x%x", res);
	res = TEEC_OpenSession(&ctx, &sess, &uuid,
			TEEC_LOGIN_PUBLIC, NULL, NULL, &err_origin);
	if (res != TEEC_SUCCESS)
		errx(1, "TEEC_Opensession failed with code 0x%x origin 0x%x", res, err_origin);
	

	do // Begin of DO WHILE(FALSE) for error handling.
	{

		// ---------- Check for command line parameters ----------
		if (1 == argc)
		{
			fprintf(stderr, "ELTT needs an option. Use '-h' for displaying help.\n");
			break;
		}

		// Loop through parameters with getopt.
		while (-1 != (option = getopt(argc, argv, "u:G:s:S:")))
		{
			switch (option)
			{
				case 'u':
					if (0 == strcasecmp(optarg, "clear"))
					{
						request = STARTUP_CLEAR;
					}
					else if (0 == strcasecmp(optarg, "state"))
					{
						request = STARTUP_STATE;
					}
					else
					{
						ret_val = ERR_BAD_CMD;
						fprintf(stderr, "Unknown option. Use '-h' for more information.\n");
					}
					
					break;
				case 'G':
					request = GET_RANDOM;
					break;
				case 's':
					request = SHA1;
					break;
				case 'S':
					request = SHA256;
					break;
				default:
					if ('u' == optopt || 'G' == optopt || 's' == optopt || 'S' == optopt)
					{
						// Error output if arguments are missing.
						fprintf(stderr, "Option '-%c' requires additional arguments. Use '-h' for more information.\n", optopt);
					}
					else if (isprint(optopt))
					{
						// Unknown parameter.
						fprintf(stderr, "Unknown option '-%c'. Use '-h' for more information.\n", optopt);
					}
					else
					{
						// Non-printable character.
						fprintf(stderr, "Invalid command line character. Use '-h' for more information.\n");
					}
					break;
			}
			
			if(request!=0){
				memset(&op, 0, sizeof(op));
				op.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_INOUT, TEEC_MEMREF_TEMP_INOUT, 
											TEEC_MEMREF_TEMP_INOUT, TEEC_MEMREF_TEMP_INOUT);
				
				op.params[0].tmpref.buffer = &request;
				op.params[0].tmpref.size = sizeof(unsigned int);
				
				op.params[1].tmpref.buffer = optarg;
				op.params[1].tmpref.size = strlen(optarg);	
					
				op.params[2].tmpref.buffer = command;
				op.params[2].tmpref.size = COMMAND_MAX_SIZE;

				op.params[3].tmpref.buffer = &command_size;
				op.params[3].tmpref.size = sizeof(unsigned int);
				
				res = TEEC_InvokeCommand(&sess, TA_MYTA_TPM_CREATE_COMMAND, &op,
								 &err_origin);
								 
				if (res != TEEC_SUCCESS)
					errx(1, "TEEC_InvokeCommand failed with code 0x%x origin 0x%x",
							res, err_origin);
				uint8_t *ptr = command;
				printf("command size : %d\n", command_size);
				for(int i=0; i<command_size; i++){
					printf("command[%d] : 0x%x\n", i, *ptr);
					ptr++;
				}
				
				tpm_response_buf_size = TPM_RESP_MAX_SIZE;
				tpm_response_buf = malloc(tpm_response_buf_size);
				MALLOC_ERROR_CHECK(tpm_response_buf);
				memset(tpm_response_buf, 0xFF, tpm_response_buf_size);
				
				tpm_value_test_buf = malloc(tpm_response_buf_size);
				MALLOC_ERROR_CHECK(tpm_value_test_buf);
				memset(tpm_value_test_buf, 0xFF, tpm_response_buf_size);
		
				ret_val = tpmtool_transmit(command, command_size, tpm_response_buf, &tpm_response_buf_size);
				// response values is 0x01(dummy data), because real data is writen in secure memory.
				printf("tpm_response_buf_size : %d\n", tpm_response_buf_size);
				ret_val = response_print(tpm_response_buf, tpm_response_buf_size, option);
				printf("\n");
				
				memset(&op, 0, sizeof(op));
				op.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_INOUT, TEEC_MEMREF_TEMP_INOUT, TEEC_MEMREF_TEMP_INOUT, TEEC_MEMREF_TEMP_INOUT);
				
				uint8_t command_log_test[COMMAND_MAX_SIZE];
				
				op.params[0].tmpref.buffer = &request;
				op.params[0].tmpref.size = sizeof(unsigned int);
				
				op.params[1].tmpref.buffer = &tpm_response_buf_size;
				op.params[1].tmpref.size = sizeof(ssize_t);
				
				op.params[2].tmpref.buffer = tpm_value_test_buf;
				op.params[2].tmpref.size = tpm_response_buf_size;
				
				op.params[3].tmpref.buffer = command_log_test;
				op.params[3].tmpref.size = COMMAND_MAX_SIZE;
				
					 
				res = TEEC_InvokeCommand(&sess, TA_MYTA_TPM_VERIFY_LOG_AND_USE_TPM_VALUE, &op,
								 &err_origin);
				
				if (res != TEEC_SUCCESS)
					errx(1, "TEEC_InvokeCommand failed with code 0x%x origin 0x%x",
							res, err_origin);
				else
					mytee_ioctl_request_unshield_spi();
				
				/*	// for test
				uint8_t *ptr2 = command_log_test;
				for(int i=0; i<100; i++){
					printf("command_log[%d] : 0x%x\n", i, *ptr2);
					ptr2++;
				}
				*/	// for test
				// for test
				printf("tpm value test buf : \n");
				ret_val = response_print(tpm_value_test_buf, tpm_response_buf_size, option);
				// for test
				
			}
		}
		for (i = optind; i < argc; i++)
		{
			fprintf(stderr, "Non-option argument '%s'. Use '-h' for more information.\n", argv[i]);
		}
	} while (0); // End of DO WHILE FALSE loop.

	TEEC_CloseSession(&sess);
	TEEC_FinalizeContext(&ctx);
		
	printf("\n");
//	end_point = clock();
	end_point = getmicsec();
	printf("Exe time : %llu us\n", end_point - start_point);
	return 0;
}

void mytee_ioctl_request_unshield_spi(){
	int fd;
 
	if ((fd = open("/dev/mytee_mod", O_RDWR)) < 0){
		printf("Cannot open /dev/mytee_mod. Try again later.\n");
	}
 
	if (ioctl(fd, MYTEE_IOCTL_REQUEST_UNSHIELD_SPI, NULL)){
		printf("Cannot write there.\n");
	}
 
	if (close(fd) != 0){
		printf("Cannot close.\n");
	}
}

int tpmtool_transmit(const uint8_t *buf, ssize_t length, uint8_t *response, ssize_t *resp_length)
{
	// ---------- Transmit command given in buf to device with handle given in dev_tpm ----------
	int ret_val = EXIT_SUCCESS;	// Return value.
	int dev_tpm = -1;		// TPM device handle.
	ssize_t transmit_size = 0;	// Amount of bytes sent to / received from the TPM.

	do
	{
		// Check input parameters.
		NULL_POINTER_CHECK(buf);
		NULL_POINTER_CHECK(response);
		NULL_POINTER_CHECK(resp_length);

		if (0 >= length)
		{
			ret_val = EINVAL;
			fprintf(stderr, "Bad parameter. Value of parameter 'length' must be larger than 0.");
			break;
		}
		if (TPM_REQ_MAX_SIZE < length)
		{
			ret_val = EINVAL;
			fprintf(stderr, "Bad parameter. Value of parameter 'length' must be smaller than or equal to %u.", TPM_REQ_MAX_SIZE);
			break;
		}
		if (TPM_CMD_HEADER_SIZE >= *resp_length)
		{
			ret_val = EINVAL;
			fprintf(stderr, "Bad parameter. Value of parameter '*resp_length' must be at least %u.", TPM_CMD_HEADER_SIZE);
			break;
		}
		if (TPM_RESP_MAX_SIZE < *resp_length)
		{
			ret_val = EINVAL;
			fprintf(stderr, "Bad parameter. Value of parameter '*resp_length' must be smaller than or equal to %u.", TPM_RESP_MAX_SIZE);
			break;
		}

		memset(response, 0, *resp_length);

		// ---------- Open TPM device ----------
		dev_tpm = open("/dev/tpm0", O_RDWR);
		if (-1 == dev_tpm)
		{
			ret_val = errno;
			fprintf(stderr, "Error opening the device.\n");
			break;
		}

		// Send request data to TPM.
		transmit_size = write(dev_tpm, buf, length);
		if (transmit_size == ERR_COMMUNICATION || length != transmit_size)
		{
			ret_val = errno;
			fprintf(stderr, "Error sending request to TPM.\n");
			break;
		}

// READ operation must called by TA.

		// Read the TPM response header.
		transmit_size = read(dev_tpm, response, TPM_RESP_MAX_SIZE);
		if (transmit_size == ERR_COMMUNICATION)
		{
			ret_val = errno;
			fprintf(stderr, "Error reading response from TPM.\n");
			break;
		}

		// Update response buffer length with value of data length returned by TPM.
		*resp_length = transmit_size;
	} while (0);

	// ---------- Close TPM device ----------
	if (-1 != dev_tpm)
	{
		// Close file handle.
		close(dev_tpm);

		// Invalidate file handle.
		dev_tpm = -1;
	}

	return ret_val;
}

static int response_print(uint8_t *response_buf, size_t resp_size, int option)
{
	int ret_val = EXIT_SUCCESS;	// Return value.

	do
	{
		NULL_POINTER_CHECK(response_buf);

		if (0 >= resp_size)
		{
			ret_val = EINVAL;
			fprintf(stderr, "Bad parameter. Value of parameter 'resp_size' must be larger than 0.");
			break;
		}
		if (TPM_RESP_MAX_SIZE < resp_size)
		{
			ret_val = EINVAL;
			fprintf(stderr, "Bad parameter. Value of parameter 'resp_size' must be smaller than or equal to %u.\n", TPM_RESP_MAX_SIZE);
			break;
		}

		switch (option)
		{
			case 'G': // Print the returned random value in hex.
				printf("Random value:\n");
				ret_val = print_response_buf(response_buf, resp_size, PRINT_RESPONSE_WITHOUT_HEADER, PRINT_RESPONSE_HEX_BLOCK);
				break;

			case 's':
			case 'S': // Print the returned hash number.
				ret_val = print_response_buf(response_buf, resp_size, PRINT_RESPONSE_WITHOUT_HEADER, PRINT_RESPONSE_HASH);
				break;
			case 'u': // Print "Startup works as expected."
				printf("Startup works as expected.\n");
				break;

			default: // Print "Done."
				printf("Done.\n");
				break;
		}
		printf("\n");
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
			fprintf(stderr, "Bad parameter. Value of parameter 'resp_size' must be larger than 0.");
			break;
		}
		if (TPM_RESP_MAX_SIZE < resp_size)
		{
			ret_val = EINVAL;
			fprintf(stderr, "Bad parameter. Value of parameter 'resp_size' must be smaller than or equal to %u.\n", TPM_RESP_MAX_SIZE);
			break;
		}
		if (resp_size <= offset)
		{
			ret_val = EINVAL;
			fprintf(stderr, "Bad parameter. Offset %u cannot be equal or larger than input buffer size %zu.\n", offset, resp_size);
			break;
		}

		switch (format)
		{
			case PRINT_RESPONSE_CLEAR:
				for (i = 0; i < resp_size - offset; i++)
				{
					printf("%02X ", response_buf[i + offset]);
				}
				break;

			case PRINT_RESPONSE_HEADERBLOCKS:
				if (TPM_CMD_HEADER_SIZE > resp_size)
				{
					ret_val = EINVAL;
					fprintf(stderr, "Response size is too small.\n");
					break;
				}

				printf("TPM Response:\n");
				for (i = 0; i < resp_size - offset; i++)
				{
					printf("%02X ", response_buf[i + offset]);
					if (i == 1) // Bytes 0 and 1 are TPM TAG.
					{
						printf("                        TPM TAG\n");
					}
					else if (i == 5) // Bytes 2 to 5 are the response length.
					{
						printf("                  RESPONSE SIZE\n");
					}
					else if (i == 9) // Last 4 bytes in header are the TPM return code.
					{
						printf("                  RETURN CODE\n");
						if (i + 1 < resp_size - offset)
						{
							printf(" Command-specific response Data:\n");
						}
					}
					else if (i >= TPM_CMD_HEADER_SIZE && (i+1) % 10 == 0) // After all header bytes have been printed, start new line after every 10 bytes of data.
					{
						printf("\n");
					}
				}
				break;

			case PRINT_RESPONSE_HEX_BLOCK:
				for (i = 0; i < resp_size - offset; i++)
				{
					if (i % 16 == 0)
					{
						printf("\n0x%08X:   ", i);
					}
					printf("0x%02X  ", response_buf[i + offset]);
				}
				break;

			case PRINT_RESPONSE_HASH:
				ret_val = buf_to_uint64(response_buf, offset - sizeof(uint16_t), sizeof(uint16_t), &data_size, resp_size); // Data size actually is only an uint16_t and should always be stored right before the actual data
				if (data_size > resp_size - offset)
				{
					ret_val = EINVAL;
					fprintf(stderr, "Invalid response data size.\n");
					break;
				}
				for (i = 0; i < data_size; i++)
				{
					if (i % 8 == 0)
					{
						printf("\n0x%08X:   ", i);
					}
					printf("%02X  ", response_buf[i + offset]);
				}
				break;

			default:
				ret_val = EINVAL;
				fprintf(stderr, "Unknown output format.\n");
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
			fprintf(stderr, "Bad parameter. Value of parameter 'input_buffer_size' must be larger than %u.\n", offset);
			break;
		}
		if (INT_MAX < input_buffer_size)
		{
			ret_val = EINVAL;
			fprintf(stderr, "Bad parameter. Value of parameter 'input_buffer_size' must be smaller or equal to %u.\n", INT_MAX);
			break;
		}
		if (0 >= length)
		{
			ret_val = EINVAL;
			fprintf(stderr, "Bad parameter. Value of parameter 'length' must be larger than 0.\n");
			break;
		}
		if (length > input_buffer_size - offset)
		{
			ret_val = EINVAL;
			fprintf(stderr, "Bad parameter. Value of parameter 'length' must be smaller or equal to %i.\n", input_buffer_size - offset);
			break;
		}
		if (sizeof(uint64_t) < length)
		{
			ret_val = EINVAL;
			fprintf(stderr, "Bad parameter. Input buffer length remaining from offset must be smaller than %zu.\n", sizeof(uint64_t));
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

