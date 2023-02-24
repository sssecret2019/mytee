#include <err.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>  
    
#include <fcntl.h>  
#include <unistd.h>  
#include <linux/fb.h>  
#include <sys/mman.h>  
#include <sys/resource.h>
#define MYTEE_IOCTL_REQUEST_UNSHIELD_FB_AND_MBOX_MMAP 0x12

/* OP-TEE TEE client API (built by optee_client) */
#include <tee_client_api.h>

/* To the the UUID (found the the TA's h-file(s)) */
#include <myta_fb_mmap.h>
static inline unsigned long long getmicsec(){
        struct timeval tv;
        gettimeofday(&tv, NULL);
        unsigned long long micsec = 1000000*tv.tv_sec+tv.tv_usec;
        return micsec;
}

int main( int argc, char* argv[] )  
{  
	int framebuffer_fd = 0;  
	struct fb_var_screeninfo framebuffer_variable_screeninfo;  
	struct fb_fix_screeninfo framebuffer_fixed_screeninfo;  
	struct rusage usage;
	unsigned long long start_t, end_t;
	start_t = getmicsec();
	getrusage(RUSAGE_SELF, &usage);
	printf("RUSAGE S, ru_nivcsw: %d\n", usage.ru_nivcsw);

	TEEC_Result res;
	TEEC_Context ctx;
	TEEC_Session sess;
	TEEC_Operation op;
	TEEC_UUID uuid = TA_MYTA_FB_MMAP_UUID;
	uint32_t err_origin;
	 
	framebuffer_fd = open( "/dev/fb0", O_RDWR );  
	lseek(framebuffer_fd, 1000000, SEEK_SET);
	if ( framebuffer_fd <  0 ){  
		perror( "Error: cannot open framebuffer device\n" );  
		exit(1);  
	}  
    
    
	if ( ioctl(framebuffer_fd, FBIOGET_VSCREENINFO,   
            	&framebuffer_variable_screeninfo) )  
	{  
		perror( "Error: reading variable screen infomation\n" );  
		exit(1);  
	}  
	
	if ( ioctl(framebuffer_fd, FBIOGET_FSCREENINFO,   
		&framebuffer_fixed_screeninfo) )  
	{  
		perror( "Error: reading fixed screen infomation\n" );  
		exit(1);  
	}  
	 
	res = TEEC_InitializeContext(NULL, &ctx);
	if (res != TEEC_SUCCESS)
		errx(1, "TEEC_InitializeContext failed with code 0x%x", res);
	res = TEEC_OpenSession(&ctx, &sess, &uuid,
				       TEEC_LOGIN_PUBLIC, NULL, NULL, &err_origin);
	if (res != TEEC_SUCCESS)
		errx(1, "TEEC_Opensession failed with code 0x%x origin 0x%x",
			res, err_origin);
			
	memset(&op, 0, sizeof(op));
	op.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_INOUT, TEEC_NONE, TEEC_NONE, TEEC_NONE);
	
	op.params[0].tmpref.buffer = &framebuffer_variable_screeninfo;
	op.params[0].tmpref.size = sizeof(struct fb_var_screeninfo);				
					
	res = TEEC_InvokeCommand(&sess, TA_MYTA_FB_MMAP_INIT, &op,
						 &err_origin);
						 
	if (res != TEEC_SUCCESS)
		errx(1, "TEEC_InvokeCommand failed with code 0x%x origin 0x%x",
				res, err_origin);
    
	if ( ioctl(framebuffer_fd, FBIOPUT_VSCREENINFO,   		// do lock address of mailbox mmio
            	&framebuffer_variable_screeninfo) )  
	{  
		perror( "Error: reading variable screen infomation\n" );  
		exit(1);  
	}  
    
	printf( "framebuffer Display information\n" );  
	printf( " %d x %d  %d bpp\n", framebuffer_variable_screeninfo.xres,  
						framebuffer_variable_screeninfo.yres,   
						framebuffer_variable_screeninfo.bits_per_pixel );  
	int width  = framebuffer_variable_screeninfo.xres;  
	int height = framebuffer_variable_screeninfo.yres;  
	int bpp = framebuffer_variable_screeninfo.bits_per_pixel/8;  
	int xoffset = framebuffer_variable_screeninfo.xoffset;  
	int yoffset = framebuffer_variable_screeninfo.yoffset;  
	int fb_line_length = framebuffer_fixed_screeninfo.line_length;
	int fb_info[6] = { width, height, bpp, xoffset, yoffset, fb_line_length };
    
	long int screensize = width*height*bpp;  
	printf("%d\n", width);
	printf("%d\n", height);
	printf("%d\n", bpp);
	printf("%d\n", xoffset);
	printf("%d\n", yoffset);
	printf("%d\n", fb_line_length);
	
	// When mmap is called on the proxy CA by the request of the TA(trusted display flag on), mmap returns the physical address of the frame buffer. 
	// If the request is normal (trusted display flag off), mmap returns the virtual address to which the framebuffer is mapped.
	// If verification fails, mmap returns -1
	unsigned long *framebuffer_physaddr = (unsigned long*)mmap( 0, screensize,  
                                        PROT_READ|PROT_WRITE,  
                                        MAP_SHARED,  
                                        framebuffer_fd, 0 ); 
	//printf("framebuffer phys_addr : %x\n", framebuffer_physaddr);
                                        
	memset(&op, 0, sizeof(op));
	op.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_INOUT, TEEC_NONE, TEEC_NONE, TEEC_NONE);

	op.params[0].tmpref.buffer = fb_info;
	op.params[0].tmpref.size = sizeof(fb_info);
	
	usleep(100000);
	
	res = TEEC_InvokeCommand(&sess, TA_MYTA_FB_MMAP_TRUSTED_DISPLAY_START, &op,
							 &err_origin);
	if (res != TEEC_SUCCESS){
		TEEC_CloseSession(&sess);
		TEEC_FinalizeContext(&ctx);
		close( framebuffer_fd );  
		mytee_ioctl_request_unshield_fb_and_mbox_mmap();		 
		errx(1, "TEEC_InvokeCommand failed with code 0x%x origin 0x%x",
				res, err_origin);			// return 1
	}
	else
		mytee_ioctl_request_unshield_fb_and_mbox_mmap();		// The framebuffer cannot be used in REE until unlocked.
		
	TEEC_CloseSession(&sess);
	TEEC_FinalizeContext(&ctx);

	close( framebuffer_fd );      

	end_t = getmicsec();

	printf("fb mmap with TZ time %llu in us, mmap_addr:%lx\n", end_t - start_t, framebuffer_physaddr); 
}

void mytee_ioctl_request_unshield_fb_and_mbox_mmap(){
	int fd;
 
	if ((fd = open("/dev/mytee_mod", O_RDWR)) < 0){
		printf("Cannot open /dev/mytee_mod. Try again later.\n");
	}
 
	if (ioctl(fd, MYTEE_IOCTL_REQUEST_UNSHIELD_FB_AND_MBOX_MMAP, NULL)){
		printf("Cannot write there.\n");
	}
 
	if (close(fd) != 0){
		printf("Cannot close.\n");
	}
}
