cp ./out/boot/* /media/`whoami`/boot
sudo cp -r ./out/rootfs/* /media/`whoami`/rootfs
sudo cp ./mytee_examples/myta_fb_mmap/host/myta_fb_mmap_no_ta /media/`whoami`/rootfs/bin
sudo cp ./mytee_examples/myta_fb_write/myta_fb_write_no_ta /media/`whoami`/rootfs/bin
sudo cp ./mytee_examples/tpm_orig/eltt2 /media/`whoami`/rootfs/bin/myta_tpm_no_ta
cp linux/arch/arm/boot/dts/*.dtb /media/`whoami`/boot
sync
