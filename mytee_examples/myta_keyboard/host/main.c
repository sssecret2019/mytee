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

#include <err.h>
#include <stdio.h>
#include <string.h>

/* OP-TEE TEE client API (built by optee_client) */
#include <tee_client_api.h>
#include <myta_keyboard.h>
#include <fcntl.h>

/* To the the UUID (found the the TA's h-file(s)) */
#include <sys/time.h>
#include <getopt.h>
#include <stdlib.h>

#include <keyboard_input_keymap.h>

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

	TEEC_Result res;
	TEEC_Context ctx;
	TEEC_Session sess;
	TEEC_Operation op;
	TEEC_UUID uuid = TA_MYTA_KEYBOARD_UUID;
	uint32_t err_origin;

	res = TEEC_InitializeContext(NULL, &ctx);
	if (res != TEEC_SUCCESS)
		errx(1, "TEEC_InitializeContext failed with code 0x%x", res);
		
	res = TEEC_OpenSession(&ctx, &sess, &uuid,
			TEEC_LOGIN_PUBLIC, NULL, NULL, &err_origin);
	if (res != TEEC_SUCCESS)
		errx(1, "TEEC_Opensession failed with code 0x%x origin 0x%x", res, err_origin);
	
	memset(&op, 0, sizeof(op));
	op.paramTypes = TEEC_PARAM_TYPES(TEEC_NONE, TEEC_NONE, TEEC_NONE, TEEC_NONE);
					
	res = TEEC_InvokeCommand(&sess, TA_MYTA_TRUSTED_KEYBOARD_ON, &op,
								 &err_origin);
								 				 
	if (res != TEEC_SUCCESS)
		errx(1, "TEEC_InvokeCommand failed with code 0x%x origin 0x%x",
				res, err_origin);

	char *argv2[] = {"bash", "-c", "XAUTHORITY=$(ls -1 /home/*/.Xauthority | head -n 1 ) xset led named \"Scroll Lock\"", 0};
	char *env2[] = {"DISPLAY=:0", NULL};
	pid_t pid = fork();
	
	switch(pid)
	{
		case -1:
			perror("fork error");
			break;
		case 0:
			printf("exec success");
			execve("/bin/bash", argv2, env2);
		default:
			printf("wait led on\n");
			wait((int*)0);
	}
	
	char secure_keyboard_testbuf[4096];
	
	printf("secure_keyboard test...\ninput buf : ");
	scanf("%s", secure_keyboard_testbuf);
	printf("input buf : %s\n", secure_keyboard_testbuf);
	
	char *argv3[] = {"bash", "-c", "XAUTHORITY=$(ls -1 /home/*/.Xauthority | head -n 1 ) xset -led named \"Scroll Lock\"", 0};		
	char *env3[] = {"DISPLAY=:0", NULL};
	pid = fork();
	switch(pid)
	{
		case -1:
			perror("fork error");
			break;
		case 0:
			printf("exec success");	 
			execve("/bin/bash", argv3, env3);
		default:
			printf("wait led off\n");
			wait((int*)0);
	}
	
	usleep(100000);		// wait 0.1 second to synchronize between led request and TA flag off request
					// TODO: optimize synchronization of LED operation
					// Setting it to 0.05 second usleep(50000) causes problems.
	
	// for test to check hardware TPM results, secret values copy to REE from TEE
	char secure_kbd_test_buffer_from_TEE[4096];
	memset(secure_kbd_test_buffer_from_TEE, 0, 4096);
	unsigned int idx = 0;
	op.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_INOUT, TEEC_MEMREF_TEMP_INOUT, 
						TEEC_NONE, TEEC_NONE);
				
	op.params[0].tmpref.buffer = secure_kbd_test_buffer_from_TEE;
	op.params[0].tmpref.size = 4096;
	op.params[1].tmpref.buffer = &idx;
	op.params[1].tmpref.size = sizeof(unsigned int);
				
	res = TEEC_InvokeCommand(&sess, TA_MYTA_TRUSTED_KEYBOARD_OFF_AND_USB_KEYBOARD_INPUT, &op,
								 &err_origin);
								 
	if (res != TEEC_SUCCESS)
		errx(1, "TEEC_InvokeCommand failed with code 0x%x origin 0x%x",
				res, err_origin);
	int i=0;
	for(i=0;i<idx;i++){
		if(secure_kbd_test_buffer_from_TEE[i]==255)
			printf("Code 255 is reserved for special needs of AT keyboard driver\n");
		else if(secure_kbd_test_buffer_from_TEE[i]>0x248)
			printf("Code %d is not generic keyboard code (not supported)\n", secure_kbd_test_buffer_from_TEE[i]);
		else
			printf("for test in REE, secure key[%d] : %s\n", i, ev_key_code_keymap[secure_kbd_test_buffer_from_TEE[i]]);
	}
	
	TEEC_CloseSession(&sess);
	TEEC_FinalizeContext(&ctx);

	end_point = getmicsec();
	printf("Exe time : %llu us\n", end_point - start_point);
	return 0;
}
