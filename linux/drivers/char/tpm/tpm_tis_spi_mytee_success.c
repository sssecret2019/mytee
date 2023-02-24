/*
 * Copyright (C) 2015 Infineon Technologies AG
 * Copyright (C) 2016 STMicroelectronics SAS
 *
 * Authors:
 * Peter Huewe <peter.huewe@infineon.com>
 * Christophe Ricard <christophe-h.ricard@st.com>
 *
 * Maintained by: <tpmdd-devel@lists.sourceforge.net>
 *
 * Device driver for TCG/TCPA TPM (trusted platform module).
 * Specifications at www.trustedcomputinggroup.org
 *
 * This device driver implements the TPM interface as defined in
 * the TCG TPM Interface Spec version 1.3, revision 27 via _raw/native
 * SPI access_.
 *
 * It is based on the original tpm_tis device driver from Leendert van
 * Dorn and Kyleen Hall and Jarko Sakkinnen.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, version 2 of the
 * License.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/wait.h>
#include <linux/acpi.h>
#include <linux/freezer.h>

#include <linux/spi/spi.h>
#include <linux/gpio.h>
#include <linux/of_irq.h>
#include <linux/of_gpio.h>
#include <linux/tpm.h>
#include "tpm.h"
#include "tpm_tis_core.h"

#ifdef CONFIG_MYTEE
#include <asm/mytee.h>
#include <asm/virt.h>

#endif


#define MAX_SPI_FRAMESIZE 64

struct tpm_tis_spi_phy {
	struct tpm_tis_data priv;
	struct spi_device *spi_device;
	u8 *iobuf;
};
unsigned int count=0;
static inline struct tpm_tis_spi_phy *to_tpm_tis_spi_phy(struct tpm_tis_data *data)
{
	return container_of(data, struct tpm_tis_spi_phy, priv);
}

static int tpm_tis_spi_transfer(struct tpm_tis_data *data, u32 addr, u16 len,
				u8 *in, const u8 *out)
{
	struct tpm_tis_spi_phy *phy = to_tpm_tis_spi_phy(data);
	int ret = 0;
	int i;
	struct spi_message m;
	struct spi_transfer spi_xfer;
	u8 transfer_len;
	spi_bus_lock(phy->spi_device->master);

	while (len) {
		transfer_len = min_t(u16, len, MAX_SPI_FRAMESIZE);
		
		phy->iobuf[0] = (in ? 0x80 : 0) | (transfer_len - 1);
		phy->iobuf[1] = 0xd4;
		phy->iobuf[2] = addr >> 8;
		phy->iobuf[3] = addr;

		memset(&spi_xfer, 0, sizeof(spi_xfer));
		spi_xfer.tx_buf = phy->iobuf;
		spi_xfer.rx_buf = phy->iobuf;
		spi_xfer.len = 4;
		spi_xfer.cs_change = 1;

		spi_message_init(&m);
		spi_message_add_tail(&spi_xfer, &m);
		ret = spi_sync_locked(phy->spi_device, &m);
		if (ret < 0)
			goto exit;

		if ((phy->iobuf[3] & 0x01) == 0) {
			// handle SPI wait states
			phy->iobuf[0] = 0;

			for (i = 0; i < TPM_RETRY; i++) {
				spi_xfer.len = 1;
				spi_message_init(&m);
				spi_message_add_tail(&spi_xfer, &m);
				ret = spi_sync_locked(phy->spi_device, &m);
				if (ret < 0)
					goto exit;
				if (phy->iobuf[0] & 0x01)
					break;
			}

			if (i == TPM_RETRY) {
				ret = -ETIMEDOUT;
				goto exit;
			}
		}

		spi_xfer.cs_change = 0;
		spi_xfer.len = transfer_len;
		spi_xfer.delay_usecs = 5;

		if (in) {
			spi_xfer.tx_buf = NULL;
		} else if (out) {
			spi_xfer.rx_buf = NULL;
			memcpy(phy->iobuf, out, transfer_len);
			out += transfer_len;
		}

		spi_message_init(&m);
		spi_message_add_tail(&spi_xfer, &m);
		ret = spi_sync_locked(phy->spi_device, &m);
		if (ret < 0)
			goto exit;
		if (in) {
			printk(KERN_ALERT "count : %u\n", count);
			if(count<300){
				if(count>250){
				printk(KERN_ALERT "iobuf : %lx\n", phy->iobuf);
				printk(KERN_ALERT "in : %lx\n", in);
				printk("transfer_len : %lx", transfer_len);
				}
				memcpy(in, phy->iobuf, transfer_len);
				in += transfer_len;
				
				if(count==250){
					__asm__ __volatile__ (
						"push {r0, r14, lr} \n\t"
					);
					mytee_set_tpm_ttbr(102,0x0);
					__asm__ __volatile__ (
						"pop {r0, r14, lr} \n\t"
					);
				}
				
			}
			else{
			int i;
				printk("hypcall---------- %lx", count);
				printk("iobuf_addr : %lx", phy->iobuf);
				printk("in_addr : %lx", in);
				for(i=0; i<transfer_len; i++){
					printk("iobuf_value : %lx", *phy->iobuf);
					printk("in_value : %lx", *in);
					phy->iobuf++;
					in++;
				}
				for(i=0; i<transfer_len; i++){
					phy->iobuf--;
					in--;
				}
				printk("transfer_len : %lx", transfer_len);
				__asm__ __volatile__ (
						"dsb \n\t"
						"isb \n\t"
						"push {r0, r14, lr} \n\t"
					);
				mytee_do_read_tpm(101, phy->iobuf, transfer_len, in);
				__asm__ __volatile__ (
						"dsb \n\t"
						"isb \n\t"
						"pop {r0, r14, lr} \n\t"
					);

				printk("iobuf_addr : %lx", phy->iobuf);
				printk("in_addr : %lx", in);
				for(i=0; i<transfer_len; i++){
					printk("iobuf_value : %lx", *phy->iobuf);
					printk("in_value : %lx", *in);
					phy->iobuf++;
					in++;
				}
				for(i=0; i<transfer_len; i++){
					phy->iobuf--;
					in--;
				}
				printk("transfer_len : %lx", transfer_len);
				/*
				__asm__ __volatile__ (
						"push {r0, r14, lr} \n\t"
				);
				mytee_up_priv(99,0x0);
				memcpy(in, phy->iobuf, transfer_len);
	   			mytee_down_priv(100,0x0);
				__asm__ __volatile__ (
						"pop {r0, r14, lr} \n\t"
				);
				*/
				in += transfer_len;
			}
			count++;
		}
		
		/*	
		unsigned int ttbr0, ttbr1, httbr;
		
		asm volatile(
			"mrc 	p15, 0, %0, c2, c0, 0\n"
			:"=r"(ttbr0)::"memory"
		);
		asm volatile(
			"mrc 	p15, 0, %0, c2, c0, 1\n"
			:"=r"(ttbr1)::"memory"
		);
		unsigned int* ttbr0_val = ttbr0 + 0x80000000;
		printk(KERN_ALERT "ttbr0_vurtual : %lx\n", ttbr0_val);
		unsigned int* ttbr1_val = ttbr1 + 0x80000000;
		printk(KERN_ALERT "ttbr1_vurtual : %lx\n", ttbr1_val);
		
		if (in) {
			count++;
			printk(KERN_ALERT "count : %d\n", count);
			if(count<=300){		// 202
				memcpy(in, phy->iobuf, transfer_len);
				in += transfer_len;
				printk(KERN_ALERT "ttbr0_val : %lx\n", *ttbr0_val);
				printk(KERN_ALERT "ttbr1_val : %lx\n", *ttbr1_val);
				if(count==202){
					printk(KERN_ALERT "OKOK ttbr0_vurtual : %lx\n", ttbr0_val);
					mytee_set_tpm_ttbr(102);
					asm volatile(
						"mrc 	p15, 0, %0, c2, c0, 0\n"
						:"=r"(ttbr0)::"memory"
					);
					printk(KERN_ALERT "OKOK ttbr0 : %lx\n", count);
					printk(KERN_ALERT "OKOK ttbr0_vurtual : %lx\n", ttbr0_val);
				}
				
			}
			else{
			/*
				if(count==203) {
					printk(KERN_ALERT "@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n");
					printk(KERN_ALERT "ttbr1_vurtual : %lx\n", ttbr1_val);
					printk(KERN_ALERT "ttbr1_val : %lx\n", *ttbr1_val);
					
				printk(KERN_ALERT "ttbr0_phyaddr : %lx\n", ttbr0_val-0x80000000);
	   				printk(KERN_ALERT "ttbr0_vurtual : %lx\n", ttbr0_val);
	   				printk(KERN_ALERT "ttbr0_val : %lx\n", *ttbr0_val);
	   				
	   			}
	   			*/
			/*
				__asm__ __volatile__ (
					"push {r0, r14, lr} \n\t"
				);
				mytee_up_priv(99,0x0);
				memcpy(in, phy->iobuf, transfer_len);
	   			mytee_down_priv(100,0x0);
	   			__asm__ __volatile__ (
					"pop {r0, r14, lr} \n\t"
				);
	   			in += transfer_len;
	   			*/
/*
	   			mytee_do_read_tpm(101, phy->iobuf, transfer_len, in);
				in += transfer_len;
				
   			}
		}
		*/
		len -= transfer_len;
	}

exit:
	spi_bus_unlock(phy->spi_device->master);
	return ret;
}

static int tpm_tis_spi_read_bytes(struct tpm_tis_data *data, u32 addr,
				  u16 len, u8 *result)
{
	return tpm_tis_spi_transfer(data, addr, len, result, NULL);
}

static int tpm_tis_spi_write_bytes(struct tpm_tis_data *data, u32 addr,
				   u16 len, const u8 *value)
{
	return tpm_tis_spi_transfer(data, addr, len, NULL, value);
}

static int tpm_tis_spi_read16(struct tpm_tis_data *data, u32 addr, u16 *result)
{
	int rc;

	rc = data->phy_ops->read_bytes(data, addr, sizeof(u16), (u8 *)result);
	if (!rc)
		*result = le16_to_cpu(*result);
	return rc;
}

static int tpm_tis_spi_read32(struct tpm_tis_data *data, u32 addr, u32 *result)
{
	int rc;

	rc = data->phy_ops->read_bytes(data, addr, sizeof(u32), (u8 *)result);
	if (!rc)
		*result = le32_to_cpu(*result);
	return rc;
}

static int tpm_tis_spi_write32(struct tpm_tis_data *data, u32 addr, u32 value)
{
	value = cpu_to_le32(value);
	return data->phy_ops->write_bytes(data, addr, sizeof(u32),
					   (u8 *)&value);
}

static const struct tpm_tis_phy_ops tpm_spi_phy_ops = {
	.read_bytes = tpm_tis_spi_read_bytes,
	.write_bytes = tpm_tis_spi_write_bytes,
	.read16 = tpm_tis_spi_read16,
	.read32 = tpm_tis_spi_read32,
	.write32 = tpm_tis_spi_write32,
};

static int tpm_tis_spi_probe(struct spi_device *dev)
{
	struct tpm_tis_spi_phy *phy;
	int irq;

	phy = devm_kzalloc(&dev->dev, sizeof(struct tpm_tis_spi_phy),
			   GFP_KERNEL);
	if (!phy)
		return -ENOMEM;

	phy->spi_device = dev;

	phy->iobuf = devm_kmalloc(&dev->dev, MAX_SPI_FRAMESIZE, GFP_KERNEL);
	if (!phy->iobuf)
		return -ENOMEM;

	/* If the SPI device has an IRQ then use that */
	if (dev->irq > 0)
		irq = dev->irq;
	else
		irq = -1;

	return tpm_tis_core_init(&dev->dev, &phy->priv, irq, &tpm_spi_phy_ops,
				 NULL);
}

static SIMPLE_DEV_PM_OPS(tpm_tis_pm, tpm_pm_suspend, tpm_tis_resume);

static int tpm_tis_spi_remove(struct spi_device *dev)
{
	struct tpm_chip *chip = spi_get_drvdata(dev);

	tpm_chip_unregister(chip);
	tpm_tis_remove(chip);
	return 0;
}

static const struct spi_device_id tpm_tis_spi_id[] = {
	{"tpm_tis_spi", 0},
	{}
};
MODULE_DEVICE_TABLE(spi, tpm_tis_spi_id);

static const struct of_device_id of_tis_spi_match[] = {
	{ .compatible = "st,st33htpm-spi", },
	{ .compatible = "infineon,slb9670", },
	{ .compatible = "tcg,tpm_tis-spi", },
	{}
};
MODULE_DEVICE_TABLE(of, of_tis_spi_match);

static const struct acpi_device_id acpi_tis_spi_match[] = {
	{"SMO0768", 0},
	{}
};
MODULE_DEVICE_TABLE(acpi, acpi_tis_spi_match);

static struct spi_driver tpm_tis_spi_driver = {
	.driver = {
		.owner = THIS_MODULE,
		.name = "tpm_tis_spi",
		.pm = &tpm_tis_pm,
		.of_match_table = of_match_ptr(of_tis_spi_match),
		.acpi_match_table = ACPI_PTR(acpi_tis_spi_match),
	},
	.probe = tpm_tis_spi_probe,
	.remove = tpm_tis_spi_remove,
	.id_table = tpm_tis_spi_id,
};
module_spi_driver(tpm_tis_spi_driver);

MODULE_DESCRIPTION("TPM Driver for native SPI access");
MODULE_LICENSE("GPL");
