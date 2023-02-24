#!/bin/sh
[ ! -d "./dl" ] && {
	mkdir dl
}

[ ! -d "./out" ] && {
    mkdir out
    mkdir out/boot
    mkdir out/rootfs
    mkdir out/rootfs/bin
    mkdir out/rootfs/lib
    mkdir out/rootfs/lib/optee_armtz
}

[ -f "./dl/arm-trusted-firmware.tar.gz" ] && {
	echo  "\033[32marm-trusted-firmware package existed.\n\033[0m"
} || {
	echo  "\033[32marm-trusted-firmware downloading...\n\033[0m"
	wget -O dl/arm-trusted-firmware.tar.gz https://github.com/ARM-software/arm-trusted-firmware/archive/v2.0.tar.gz
}

[ ! -d "./arm-trusted-firmware" ] && {
	tar zxf dl/arm-trusted-firmware.tar.gz && {
		mv arm-trusted-firmware-* arm-trusted-firmware
	}
}

[ -f "./dl/u-boot.tar.gz" ] && {
	echo  "\033[32mu-boot package existed.\n\033[0m"
} || {
	echo  "\033[32mu-boot downloading...\n\033[0m"
	wget -O dl/u-boot.tar.gz https://github.com/u-boot/u-boot/archive/v2018.11.tar.gz
}

[ ! -d "./u-boot" ] && {
	tar zxf dl/u-boot.tar.gz && {
		mv u-boot-* u-boot
	}
}

[ -f "./dl/optee_os.tar.gz" ] && {
	echo  "\033[32moptee_os package existed.\n\033[0m"
} || {
	echo  "\033[32moptee_os downloading...\n\033[0m"
	wget -O dl/optee_os.tar.gz https://github.com/OP-TEE/optee_os/archive/3.4.0.tar.gz
}

[ ! -d "./optee_os" ] && {
	tar zxf dl/optee_os.tar.gz && {
		mv optee_os-* optee_os
	}
	sed -i /ta_arm32/d optee_os/core/arch/arm/plat-rpi3/conf.mk
}

[ -f "./dl/optee_client.tar.gz" ] && {
	echo  "\033[32moptee_client package existed.\n\033[0m"
} || {
	echo  "\033[32moptee_client downloading...\n\033[0m"
	wget -O dl/optee_client.tar.gz https://github.com/OP-TEE/optee_client/archive/3.3.0.tar.gz
}

[ ! -d "./optee_client" ] && {
	tar zxf dl/optee_client.tar.gz && {
		mv optee_client-* optee_client
	}
}

[ -f "./dl/optee_examples.tar.gz" ] && {
	echo  "\033[32moptee_examples package existed.\n\033[0m"
} || {
	echo  "\033[32moptee_examples downloading...\n\033[0m"
	wget -O dl/optee_examples.tar.gz https://github.com/linaro-swg/optee_examples/archive/3.3.0.tar.gz
}

[ ! -d "./optee_examples" ] && {
	tar zxf dl/optee_examples.tar.gz && {
		mv optee_examples-* optee_examples
	}
}

[ -f "./dl/linux.tar.gz" ] && {
	echo  "\033[32mlinux package existed.\n\033[0m"
} || {
	echo  "\033[32mlinux downloading...\n\033[0m"
	wget -O dl/linux.tar.gz https://github.com/raspberrypi/linux/archive/raspberrypi-kernel_1.20190215-1.tar.gz
}

[ ! -d "./linux" ] && {
	tar zxf dl/linux.tar.gz && {
		mv linux-* linux
	}
}

