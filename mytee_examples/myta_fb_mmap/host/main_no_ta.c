#include <err.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>  
    
#include <fcntl.h>  
#include <unistd.h>  
#include <linux/fb.h>  
#include <sys/mman.h>  
#include <sys/resource.h>



/* OP-TEE TEE client API (built by optee_client) */
//#include <tee_client_api.h>

/* To the the UUID (found the the TA's h-file(s)) */
//#include <myta4.h>
 
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
    unsigned long long start_t, end_t;
//    struct rusage usage;

   start_t = getmicsec();    
//   getrusage(RUSAGE_SELF, &usage);
//   printf("RUSAGE S, ru_nivcsw: %d\n", usage.ru_nivcsw);
	
    framebuffer_fd = open( "/dev/fb0", O_RDWR );  
   // lseek(framebuffer_fd, 20000000, SEEK_SET);
    
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
    framebuffer_variable_screeninfo.bits_per_pixel=32;  
    
    if ( ioctl(framebuffer_fd, FBIOPUT_VSCREENINFO,   
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
/*
	TEEC_Result res;
	TEEC_Context ctx;
	TEEC_Session sess;
	TEEC_Operation op;
	TEEC_UUID uuid = TA_MYTA4_UUID;
	uint32_t err_origin;
*/
/*	
    unsigned long *framebuffer_phyaddr;
		framebuffer_phyaddr = malloc(4);
		memset(framebuffer_phyaddr, 0xFF, 4);
    

    framebuffer_phyaddr = 0x3e402000;
    printf("phy_addr : %lx\n", framebuffer_phyaddr);
    printf("\n\n");
 
    		res = TEEC_InitializeContext(NULL, &ctx);
		if (res != TEEC_SUCCESS)
			errx(1, "TEEC_InitializeContext failed with code 0x%x", res);
		res = TEEC_OpenSession(&ctx, &sess, &uuid,
				       TEEC_LOGIN_PUBLIC, NULL, NULL, &err_origin);
		if (res != TEEC_SUCCESS)
			errx(1, "TEEC_Opensession failed with code 0x%x origin 0x%x",
				res, err_origin);
		memset(&op, 0, sizeof(op));
		op.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_TEMP_INOUT, 					TEEC_MEMREF_TEMP_INOUT, TEEC_NONE, TEEC_NONE);
		
		op.params[0].tmpref.buffer = &framebuffer_phyaddr;
		op.params[0].tmpref.size = 4;
		op.params[1].tmpref.buffer = fb_info;
		op.params[1].tmpref.size = sizeof(fb_info);
		
		printf("invokecommand");		
		res = TEEC_InvokeCommand(&sess, TA_MYTA4_CMD_INC_VALUE, &op,
						 &err_origin);
		if (res != TEEC_SUCCESS)
			errx(1, "TEEC_InvokeCommand failed with code 0x%x origin 0x%x",
					res, err_origin);
		TEEC_CloseSession(&sess);
		TEEC_FinalizeContext(&ctx);
    munmap( framebuffer_fd, screensize ); 
    close( framebuffer_fd );      
      */ 
       
    /*
    framebuffer_phyaddr = (unsigned long*)mmap( 0, screensize,  
                                        PROT_READ|PROT_WRITE,  
                                        MAP_SHARED,  
                                        framebuffer_fd, 0 ); 
                                        */    
 
// ################ using shared memory ########################

    char *framebuffer_pointer = (char*)mmap( 0, screensize,  
                                        PROT_READ|PROT_WRITE,  
                                        MAP_SHARED,  
                                        framebuffer_fd, 0 ); 

	
/*    
                
    	TEEC_Result res;
	TEEC_Context ctx;
	TEEC_Session sess;
	TEEC_Operation op;
	TEEC_UUID uuid = TA_MYTA4_UUID;
	uint32_t err_origin;
	TEEC_SharedMemory shm;
	
	shm.flags = TEEC_MEM_INPUT | TEEC_MEM_OUTPUT;
	shm.size = 0x7e9000;
	shm.buffer = framebuffer_pointer;
	
        TEEC_InitializeContext(NULL, &ctx);
	TEEC_OpenSession(&ctx, &sess, &uuid,
			       TEEC_LOGIN_PUBLIC, NULL, NULL, &err_origin);
	memset(&op, 0, sizeof(op));
	op.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_WHOLE, TEEC_MEMREF_TEMP_INOUT, TEEC_NONE, TEEC_NONE);

	// regester shared memory address of framebuffer_pointer.. mytee

    	
	op.params[0].memref.parent = &shm;
	op.params[0].memref.size = shm.size;
	TEEC_RegisterSharedMemory(&ctx, &shm);
	
	op.params[1].tmpref.buffer = fb_info;
	op.params[1].tmpref.size = sizeof(fb_info);
	
	res = TEEC_InvokeCommand(&sess, TA_MYTA4_CMD_INC_VALUE, &op,
					 &err_origin);	
	TEEC_CloseSession(&sess);
	TEEC_FinalizeContext(&ctx);

*/
// emulate this code in TA..
	
    if ( framebuffer_pointer == MAP_FAILED )  
    {  
        perror( "Error : mmap\n" );  
        exit(1);  
    }  
    else  
        {  
/*            int x,y;  
            for ( y=0; y<height; y++)  
            	for ( x=0; x<width; x++)  
            	{  
               	 unsigned int pixel_offset = (y+yoffset)*framebuffer_fixed_screeninfo.line_length*2 +(x+xoffset)*bpp;  
   //     		printf("%d\n", framebuffer_pointer[pixel_offset]);
    
			 if (bpp==4){  
				    if ( x<=width*1/3){    
					 framebuffer_pointer[pixel_offset]=255;//B  
					 framebuffer_pointer[pixel_offset+1]=0;//G  
					 framebuffer_pointer[pixel_offset+2]=0;//R  
					 framebuffer_pointer[pixel_offset+3]=0;//A  
				    }  
				    if ( x>width*1/3 && x<=width*2/3){      
					 framebuffer_pointer[pixel_offset]=0;//B  
					 framebuffer_pointer[pixel_offset+1]=255;//G  
					 framebuffer_pointer[pixel_offset+2]=0;//R  
					 framebuffer_pointer[pixel_offset+3]=0;//A  
				    }  
				    if ( x>width*2/3){     
					 framebuffer_pointer[pixel_offset]=0;//B  
					 framebuffer_pointer[pixel_offset+1]=0;//G  
					 framebuffer_pointer[pixel_offset+2]=255;//R  
					 framebuffer_pointer[pixel_offset+3]=0;//A  
        			    }  
      			  }  
    
       	 }  
    */
int    	        size2 = 4096 * 400;
      size2 = 4096 * 800;
      size2 = 4096 * 1600;     
        for(int i=0 ; i < size2;i++)
                framebuffer_pointer[i] = i;
	
	}   
	
    munmap( framebuffer_pointer, screensize );   
    close( framebuffer_fd );  
//	getrusage(RUSAGE_SELF, &usage);
    end_t = getmicsec();
//        printf("RUSAGE E, ru_nivcsw: %d\n", usage.ru_nivcsw);	
    printf("FB MMAP without TZ time %llu in us\n", end_t - start_t); 
}
