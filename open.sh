#Open frequently used files related to initial settings
gedit ./linux/init/main.c
gedit ./arm-trusted-firmware/plat/rpi3/rpi3_common.c
gedit ./linux/System.map
gedit ./linux/arch/arm/include/asm/mytee.h
gedit ./linux/arch/arm/kernel/mytee.S
gedit ./linux/arch/arm/include/asm/virt.h
gedit ./linux/arch/arm/kernel/hyp-stub.S
gedit ./arm-trusted-firmware/bl31/aarch64/runtime_exceptions.S

#Open frequently used files related to mytee kernel module
gedit ./linux/drivers/mytee_mod/mytee_mod.c

#Open frequently used files related to secure TPM
gedit ./mytee_examples/myta_tpm/host/main.c
gedit ./optee_os/core/arch/arm/pta/myta_tpm.c
gedit ./optee_os/core/arch/arm/include/myta_tpm.h
gedit ./linux/drivers/char/tpm/tpm-dev-common.c
gedit ./linux/drivers/char/tpm/tpm_tis_spi_mytee.c
gedit ./linux/drivers/spi/spi-bcm2835.c

#Open frequently used files related to trusted USB keyboard
gedit ./mytee_examples/myta_keyboard/host/main.c
gedit ./optee_os/core/arch/arm/pta/myta_keyboard.c
gedit ./optee_os/core/arch/arm/include/myta_keyboard.h
gedit ./linux/drivers/usb/core/hub.c
gedit ./linux/drivers/hid/usbhid/hid-core.c
gedit ./linux/drivers/hid/hid-core.c

#Open frequently used files related DMA (DMA Filter is implemented in hyp-stub.S)
gedit ./linux/drivers/usb/host/dwc_otg/dwc_otg_hcd.c
gedit ./linux/drivers/usb/host/dwc_otg/dwc_otg_hcd_ddma.c
gedit ./linux/drivers/usb/host/dwc_otg/dwc_otg_cil.c

#Open frequently used files related to trusted display
gedit ./mytee_examples/myta_fb_mmap/host/main.c
gedit ./optee_os/core/arch/arm/pta/myta_fb_mmap.c
gedit ./optee_os/core/arch/arm/include/myta_fb_mmap.h
gedit ./mytee_examples/myta_fb_write/host/main.c
gedit ./optee_os/core/arch/arm/pta/myta_fb_write.c
gedit ./optee_os/core/arch/arm/include/myta_fb_write.h
gedit ./linux/drivers/video/fbdev/core/fbcon.c
gedit ./linux/drivers/video/fbdev/core/fbmem.c
gedit ./linux/drivers/mailbox/bcm2835-mailbox.c
gedit ./linux/drivers/firmware/raspberrypi.c
gedit ./linux/drivers/cpufreq/bcm2835-cpufreq.c
