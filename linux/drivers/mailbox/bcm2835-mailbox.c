/*
 *  Copyright (C) 2010,2015 Broadcom
 *  Copyright (C) 2013-2014 Lubomir Rintel
 *  Copyright (C) 2013 Craig McGeachie
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This device provides a mechanism for writing to the mailboxes,
 * that are shared between the ARM and the VideoCore processor
 *
 * Parts of the driver are based on:
 *  - arch/arm/mach-bcm2708/vcio.c file written by Gray Girling that was
 *    obtained from branch "rpi-3.6.y" of git://github.com/raspberrypi/
 *    linux.git
 *  - drivers/mailbox/bcm2835-ipc.c by Lubomir Rintel at
 *    https://github.com/hackerspace/rpi-linux/blob/lr-raspberry-pi/drivers/
 *    mailbox/bcm2835-ipc.c
 *  - documentation available on the following web site:
 *    https://github.com/raspberrypi/firmware/wiki/Mailbox-property-interface
 */

#include <linux/device.h>
#include <linux/dma-mapping.h>
#include <linux/err.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/kernel.h>
#include <linux/mailbox_controller.h>
#include <linux/module.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/platform_device.h>
#include <linux/spinlock.h>
#ifdef CONFIG_MYTEE
#include <asm/mytee.h>
#include <asm/virt.h>
unsigned long mbox_mmio_virtaddr = 0;
EXPORT_SYMBOL(mbox_mmio_virtaddr);
#define MBOX_MMIO_PHYSADDR 0x3f00b880
#endif
/* Mailboxes */
#define ARM_0_MAIL0	0x00
#define ARM_0_MAIL1	0x20

/*
 * Mailbox registers. We basically only support mailbox 0 & 1. We
 * deliver to the VC in mailbox 1, it delivers to us in mailbox 0. See
 * BCM2835-ARM-Peripherals.pdf section 1.3 for an explanation about
 * the placement of memory barriers.
 */
#define MAIL0_RD	(ARM_0_MAIL0 + 0x00)
#define MAIL0_POL	(ARM_0_MAIL0 + 0x10)
#define MAIL0_STA	(ARM_0_MAIL0 + 0x18)
#define MAIL0_CNF	(ARM_0_MAIL0 + 0x1C)
#define MAIL1_WRT	(ARM_0_MAIL1 + 0x00)
#define MAIL1_STA	(ARM_0_MAIL1 + 0x18)

/* On ARCH_BCM270x these come through <linux/interrupt.h> (arm_control.h ) */
#ifndef ARM_MS_FULL
/* Status register: FIFO state. */
#define ARM_MS_FULL		BIT(31)
#define ARM_MS_EMPTY		BIT(30)

/* Configuration register: Enable interrupts. */
#define ARM_MC_IHAVEDATAIRQEN	BIT(0)
#endif

struct bcm2835_mbox {
	void __iomem *regs;
	spinlock_t lock;
	struct mbox_controller controller;
};

static struct bcm2835_mbox *bcm2835_link_mbox(struct mbox_chan *link)
{
	return container_of(link->mbox, struct bcm2835_mbox, controller);
}

static irqreturn_t bcm2835_mbox_irq(int irq, void *dev_id)
{
	struct bcm2835_mbox *mbox = dev_id;
	struct device *dev = mbox->controller.dev;
	struct mbox_chan *link = &mbox->controller.chans[0];

	while (!(readl(mbox->regs + MAIL0_STA) & ARM_MS_EMPTY)) {
		u32 msg = readl(mbox->regs + MAIL0_RD);
		dev_dbg(dev, "Reply 0x%08X\n", msg);
		mbox_chan_received_data(link, &msg);
	}
	return IRQ_HANDLED;
}
#ifdef CONFIG_MYTEE


static void mytee_copy_critical_var(u32 sec_msg, u32 msg){
	u32 *source, *dest;
	dest = sec_msg;
	source = (u32 *)(msg - (BCM2835_VIRT_TO_BUS_OFFSET | 0x8)); //Convert to the the virtual addr. 0x8 is channel number (refer to the MBOX_MSG macro)
	u32 len = source[0];
	memcpy(dest, source, len);
	wmb();
}

static int mytee_verify_write(u32 context, u32 msg, void * dest, u32 size, u32 mbox_mmio_phys){
	int ret = 0;

	if(mbox_mmio_phys != MBOX_MMIO_PHYSADDR){
		ret = 1;
		return ret;
	}
	
	if(msg != (MYTEE_HYP_SECURE_MAIL_BOX_MSG_VIRT + (BCM2835_VIRT_TO_BUS_OFFSET | 0x8))){
		printk("MYTEE ERROR!!!, msg: %lx", msg);
		dump_stack();
		ret = 1;	
		return ret;
	}
	return ret;
}

static int mytee_log_write(u8 context, u32 sec_msg){
	int i=0;
	u32 * log_ctx = (u32 *)MYTEE_TRUSTED_FB_MAIL_BOX_LOG_CONTEXT_VIRT;
	u32 * log_page = (u32 *)MYTEE_TRUSTED_FB_MAIL_BOX_LOG_PAGE_VIRT;
	u32* sec_msg_ptr = (u32 *)sec_msg;
	
	*log_ctx = (u32)context;
	u32 msg_size = *sec_msg_ptr;
	
	for(i=0; i<msg_size/4; i++){
		*log_page = *sec_msg_ptr;
		log_page++;
		sec_msg_ptr++;
		asm volatile("mcr p15, 0, %0, c7, c6, 1"::"r"(log_page));	// dcache invalidate mva
	}
	*log_ctx = *log_ctx | MYTEE_CONTEXT_MBOX_LOG_COMPLETE;
		
	asm volatile("mcr p15, 0, %0, c7, c6, 1"::"r"(log_ctx));	// dcache invalidate mva
	
	return 0;
}
EXPORT_SYMBOL(mytee_log_write);

static int mytee_wrapper_write(u8 context, u32 msg, void * dest, u32 mbox_mmio_phys){
	u32 sec_msg = MYTEE_HYP_SECURE_MAIL_BOX_MSG_VIRT;
	int ret;

	ret = mytee_verify_write(context, msg, dest, sizeof(sec_msg), mbox_mmio_phys);
	writel(msg, (void __iomem *)dest);	
	mytee_log_write(context, sec_msg);
	
	return ret;
}
#endif
static int bcm2835_send_data(struct mbox_chan *link, void *data)
{
	struct bcm2835_mbox *mbox = bcm2835_link_mbox(link);
	u32 msg = *(u32 *)data;

	spin_lock(&mbox->lock);
#ifdef CONFIG_MYTEE
        u32 mbox_mmio_phys = mytee_get_kernel_phys(MYTEE_GET_KERNEL_PHYS, mbox->regs, 0, 0);
	mytee_up_priv(MYTEE_UP_PRIV,0,0,0); //Logging the context
	u32* trusted_display_mmap_flag = MYTEE_TRUSTED_FB_MMAP_FLAG_VIRT;
	u32* trusted_display_write_flag = MYTEE_TRUSTED_FB_WRITE_FLAG_VIRT;
	u32* verify_ret = MYTEE_TRUSTED_FB_VERIFY_RET_VIRT;
	u8* context = MYTEE_TRUSTED_FB_CONTEXT_VIRT;
	if(*trusted_display_mmap_flag==0x1)
		*context = MYTEE_CONTEXT_MAILBOX_MMAP;		// Notify that the trusted display is ON, to bcm2835-mailbox.c
	else if(*trusted_display_write_flag==0x1)
		*context = MYTEE_CONTEXT_MAILBOX_WRITE;
	mytee_down_priv(MYTEE_DOWN_PRIV,0);
	if(*context==MYTEE_CONTEXT_MAILBOX_MMAP || *context==MYTEE_CONTEXT_MAILBOX_WRITE){
		mytee_up_priv(MYTEE_UP_PRIV,0,0,0); // Logging the context
		*verify_ret = mytee_wrapper_write(*context, msg, mbox->regs + MAIL1_WRT, mbox_mmio_phys);
		mytee_down_priv(MYTEE_DOWN_PRIV,0);
		if(*verify_ret!=0){
			*verify_ret = 0x0;
			printk("MYTEE: verify fail..");
			return -1;
		}
		*verify_ret=0x0;
	}
        else{
        	writel(msg, mbox->regs + MAIL1_WRT);
        }
#else
	writel(msg, mbox->regs + MAIL1_WRT);
#endif
exit:
	dev_dbg(mbox->controller.dev, "Request 0x%08X\n", msg);
	spin_unlock(&mbox->lock);
	return 0;
}

static int bcm2835_startup(struct mbox_chan *link)
{
	struct bcm2835_mbox *mbox = bcm2835_link_mbox(link);

	/* Enable the interrupt on data reception */
	writel(ARM_MC_IHAVEDATAIRQEN, mbox->regs + MAIL0_CNF);

	return 0;
}

static void bcm2835_shutdown(struct mbox_chan *link)
{
	struct bcm2835_mbox *mbox = bcm2835_link_mbox(link);

	writel(0, mbox->regs + MAIL0_CNF);
}

static bool bcm2835_last_tx_done(struct mbox_chan *link)
{
	struct bcm2835_mbox *mbox = bcm2835_link_mbox(link);
	bool ret;

	spin_lock(&mbox->lock);
	ret = !(readl(mbox->regs + MAIL1_STA) & ARM_MS_FULL);
	spin_unlock(&mbox->lock);
	return ret;
}

static const struct mbox_chan_ops bcm2835_mbox_chan_ops = {
	.send_data	= bcm2835_send_data,
	.startup	= bcm2835_startup,
	.shutdown	= bcm2835_shutdown,
	.last_tx_done	= bcm2835_last_tx_done
};

static struct mbox_chan *bcm2835_mbox_index_xlate(struct mbox_controller *mbox,
		    const struct of_phandle_args *sp)
{
	if (sp->args_count != 0)
		return NULL;

	return &mbox->chans[0];
}

static int bcm2835_mbox_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	int ret = 0;
	struct resource *iomem;
	struct bcm2835_mbox *mbox;

	mbox = devm_kzalloc(dev, sizeof(*mbox), GFP_KERNEL);
	if (mbox == NULL)
		return -ENOMEM;
	spin_lock_init(&mbox->lock);

	ret = devm_request_irq(dev, platform_get_irq(pdev, 0),
			       bcm2835_mbox_irq, 0, dev_name(dev), mbox);
	if (ret) {
		dev_err(dev, "Failed to register a mailbox IRQ handler: %d\n",
			ret);
		return -ENODEV;
	}

	iomem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	mbox->regs = devm_ioremap_resource(&pdev->dev, iomem);
	mbox_mmio_virtaddr = mbox->regs;
	
	if (IS_ERR(mbox->regs)) {
		ret = PTR_ERR(mbox->regs);
		dev_err(&pdev->dev, "Failed to remap mailbox regs: %d\n", ret);
		return ret;
	}

	mbox->controller.txdone_poll = true;
	mbox->controller.txpoll_period = 5;
	mbox->controller.ops = &bcm2835_mbox_chan_ops;
	mbox->controller.of_xlate = &bcm2835_mbox_index_xlate;
	mbox->controller.dev = dev;
	mbox->controller.num_chans = 1;
	mbox->controller.chans = devm_kzalloc(dev,
		sizeof(*mbox->controller.chans), GFP_KERNEL);
	if (!mbox->controller.chans)
		return -ENOMEM;

	ret = mbox_controller_register(&mbox->controller);
	if (ret)
		return ret;

	platform_set_drvdata(pdev, mbox);
	dev_info(dev, "mailbox enabled\n");

	return ret;
}

static int bcm2835_mbox_remove(struct platform_device *pdev)
{
	struct bcm2835_mbox *mbox = platform_get_drvdata(pdev);
	mbox_controller_unregister(&mbox->controller);
	return 0;
}

static const struct of_device_id bcm2835_mbox_of_match[] = {
	{ .compatible = "brcm,bcm2835-mbox", },
	{},
};
MODULE_DEVICE_TABLE(of, bcm2835_mbox_of_match);

static struct platform_driver bcm2835_mbox_driver = {
	.driver = {
		.name = "bcm2835-mbox",
		.of_match_table = bcm2835_mbox_of_match,
	},
	.probe		= bcm2835_mbox_probe,
	.remove		= bcm2835_mbox_remove,
};

static int __init bcm2835_mbox_init(void)
{
	return platform_driver_register(&bcm2835_mbox_driver);
}
arch_initcall(bcm2835_mbox_init);

static void __init bcm2835_mbox_exit(void)
{
	platform_driver_unregister(&bcm2835_mbox_driver);
}
module_exit(bcm2835_mbox_exit);

MODULE_AUTHOR("Lubomir Rintel <lkundrak@v3.sk>");
MODULE_DESCRIPTION("BCM2835 mailbox IPC driver");
MODULE_LICENSE("GPL v2");
