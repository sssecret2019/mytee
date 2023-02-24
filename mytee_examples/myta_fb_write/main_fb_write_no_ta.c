#include <err.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>  
    
#include <fcntl.h>  
#include <unistd.h>  
#include <linux/fb.h>  
#include <sys/mman.h>  
#include <sys/time.h>



/* OP-TEE TEE client API (built by optee_client) */

/* To the the UUID (found the the TA's h-file(s)) */
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
    start_t = getmicsec();
    
    framebuffer_fd = open( "/dev/fb0", O_RDWR );  
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


	unsigned int size = (height+yoffset)*framebuffer_fixed_screeninfo.line_length*2 +(width+xoffset)*bpp + 3*height*width;
	char *framebuffer = malloc(size);
	
	
/*            int x,y;  
            for ( y=0; y<height-1; y++)  
            	for ( x=0; x<width-1; x++)  
            	{  
               	 unsigned int pixel_offset = (y+yoffset)*framebuffer_fixed_screeninfo.line_length*2 +(x+xoffset)*bpp;  
//        		printf("%d\n", framebuffer[pixel_offset]);
    
			 if (bpp==4){  
				    if ( x<=width*1/3){    
					 framebuffer[pixel_offset]=255;//B  
					 framebuffer[pixel_offset+1]=0;//G  
					 framebuffer[pixel_offset+2]=0;//R  
					 framebuffer[pixel_offset+3]=0;//A  
				    }  
				    if ( x>width*1/3 && x<=width*2/3){      
					 framebuffer[pixel_offset]=0;//B  
					 framebuffer[pixel_offset+1]=255;//G  
					 framebuffer[pixel_offset+2]=0;//R  
					 framebuffer[pixel_offset+3]=0;//A  
				    }  
				    if ( x>width*2/3){     
					 framebuffer[pixel_offset]=0;//B  
					 framebuffer[pixel_offset+1]=0;//G  
					 framebuffer[pixel_offset+2]=255;//R  
					 framebuffer[pixel_offset+3]=0;//A  
        			    }  
      			  }  
    
       	 }  
*/  
        size = 4096 * 400;
//	size = 4096 * 800;
//	size = 4096 * 1600;	
	for(int i=0 ; i < size;i++)
		framebuffer[i] = i;
//	printf("%d\n", framebuffer[0]);
	 
	write(framebuffer_fd, framebuffer, size);	
    close( framebuffer_fd );  
    end_t = getmicsec();
    printf("FB write no TA time %llu in us\n", end_t - start_t);
    
}
