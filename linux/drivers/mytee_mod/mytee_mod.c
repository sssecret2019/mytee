#ifdef CONFIG_MYTEE
#include <linux/module.h>  
#include <linux/kernel.h>  
#include <linux/cdev.h>  
#include <linux/device.h>  
#include <linux/fs.h>            
#include <linux/slab.h>  
#include <asm/uaccess.h>  
#include <asm/mytee.h>

dev_t id;  
struct cdev cdev;  
struct class *class;  
struct device *dev;  

#define DEVICE_NAME "mytee_mod"  

int mytee_mod_open (struct inode *inode, struct file *filp)  
{      
    return 0;  
}  
    
int mytee_mod_close (struct inode *inode, struct file *filp)  
{  
    return 0;  
}  
    
long mytee_mod_ioctl ( struct file *filp, unsigned int cmd, unsigned long arg)  
{  
	switch(cmd)
	{
		case MYTEE_IOCTL_REQUEST_UNSHIELD_SPI:
			// In case of the 4KB unshield API, the flag of TA is checked in the hyp
			mytee_unshield_mmio_4kb(MYTEE_UNSHIELD_MMIO_4KB, spi_mmio_virtaddr, MYTEE_MMIO_CONTEXT_DEVICE_TPM, 0x1000);
		break;
		case MYTEE_IOCTL_REQUEST_UNSHIELD_FB_AND_MBOX_MMAP:
		{
			// In case of the 2MB unshield API, the flag of TA is not checked in hyp
			// Hence, the flag should be checked here.
			mytee_up_priv(MYTEE_UP_PRIV,0,0,0);
			u32* trusted_display_mmap_flag = MYTEE_TRUSTED_FB_MMAP_FLAG_VIRT;
			if(*trusted_display_mmap_flag==0x0){
				mytee_down_priv(MYTEE_DOWN_PRIV,0);
				mytee_remmap_fb(MYTEE_REMMAP_FB, 0, 0);
				mytee_unshield_mmio_4kb(MYTEE_UNSHIELD_MMIO_4KB, mbox_mmio_virtaddr, MYTEE_MMIO_CONTEXT_DEVICE_FRAMEBUFFER, 0x1000);
				//mytee_unshield_mmio_with_phys(115, fb_phys_addr, 0, fb_size);
				break;
			}
			else{
				mytee_down_priv(MYTEE_DOWN_PRIV,0);
			}
			break;
		}
		case MYTEE_IOCTL_REQUEST_UNSHIELD_FB_WRITE:
		{
			// In case of the 2MB unshield API, the flag of TA is not checked in hyp
			// So the flag should be check here.
			mytee_up_priv(MYTEE_UP_PRIV,0,0,0);
			u32* trusted_display_write_flag = MYTEE_TRUSTED_FB_WRITE_FLAG_VIRT;
			if(*trusted_display_write_flag==0x0){
				mytee_down_priv(MYTEE_DOWN_PRIV,0);
				mytee_remmap_fb(MYTEE_REMMAP_FB, 0, 0);
				//mytee_unshield_mmio_with_phys(115, fb_phys_addr, 0, fb_size);
				break;
			}
			else{
				mytee_down_priv(MYTEE_DOWN_PRIV,0);
			}
			break;
		}
	}
	
    return 0;  

} 
EXPORT_SYMBOL(mytee_mod_ioctl);  
    
struct file_operations simple_fops =  
{  
    .owner           = THIS_MODULE,      
    .unlocked_ioctl  = mytee_mod_ioctl,      
    .open            = mytee_mod_open,       
    .release         = mytee_mod_close,    
};  
    
int mytee_dev_init(void)  
{  
    int ret;  
    
    ret = alloc_chrdev_region( &id, 0, 1, DEVICE_NAME );  
    if ( ret ){  
        printk( "alloc_chrdev_region error %d\n", ret );  
        return ret;  
    }  
    
    cdev_init( &cdev, &simple_fops );  
    cdev.owner = THIS_MODULE;  
    
    ret = cdev_add( &cdev, id, 1 );  
    if (ret){  
        printk( "cdev_add error %d\n", ret );  
        unregister_chrdev_region( id, 1 );  
        return ret;  
    }  
    
    class = class_create( THIS_MODULE, DEVICE_NAME );  
    if ( IS_ERR(class)){  
        ret = PTR_ERR( class );  
        printk( "class_create error %d\n", ret );  
    
        cdev_del( &cdev );  
        unregister_chrdev_region( id, 1 );  
        return ret;  
    }  
    
    dev = device_create( class, NULL, id, NULL, DEVICE_NAME );  
    if ( IS_ERR(dev) ){  
        ret = PTR_ERR(dev);  
        printk( "device_create error %d\n", ret );  
    
        class_destroy(class);  
        cdev_del( &cdev );  
        unregister_chrdev_region( id, 1 );  
        return ret;  
    }  
    
    
    return 0;  
}  
    
void mytee_dev_exit(void)  
{  
    device_destroy(class, id );  
    class_destroy(class);  
    cdev_del( &cdev );  
    unregister_chrdev_region( id, 1 );  
}  
module_init(mytee_dev_init);
module_exit(mytee_dev_exit);

MODULE_LICENSE("Dual BSD/GPL");  
#endif
