Raspbian with OP-TEE Support
============================

Introduction
------------

This project discribed how to integrating OP-TEE within Raspbian.

There are two prerequisite:

- Raspbian have 32-bit version only.
- ATF(Trust Firmware-A) now only have 64-bit support for Raspberry Pi 3. (`ATF RPI3 support`_).

I don't want to change this default when considering big workload.
So, this project will build ATF as 64-bit, and all others as 32-bit.

Dependence
----------

- Host OS: Ubuntu 16.04 or later.
- Cross Build Toolchain: AARCH64 & AARCH32 both needed, and AARCH32 must > 6.0. (you can get one from `linaro`_)

- Hardware: Support only Raspberry Pi 3rd generation (such as 3B/3B+) board, not first or 2nd generation board.

Build
-----
First, config your cross build toolchain at config.mk.

Then:

.. code:: bash

	$ ./prepare-env.sh # if your had download all packages, skip this.
	$ make patch # this will patch linux kernel & ATF, if you have done before, skip this.
	$ make

When success, it should seem as:

.. image:: doc/raspbian-tee-output.jpg

Install
-------

Prepare a SD Card Flashed with an official Raspbian image, mount it in linux, such as /media/user/boot & /media/user/rootfs.

Then:

.. code:: bash

	$ cp ./out/boot/* /media/user/boot
	$ sudo cp -r ./out/rootfs/* /media/user/rootfs

Test OP-TEE is ok
-----------------

Boot Rsapberry Pi with the Modified image in SD Card.

When you are logined,then:

.. code:: bash

	$ ls /dev/tee*
	/dev/tee0 /dev/teepriv0 # this prove tee driver & optee-os works.
	$ sudo tee-supplicant &
	$ sudo optee_example_hello_world

It should be no errors, then all is OK.


TODO
----

Shell scripts should be moved to Makefile.

.. _ATF RPI3 support: https://github.com/ARM-software/arm-trusted-firmware/blob/620d9832f96ffcaf86d38b703ca913438d6eea7c/plat/rpi3/platform.mk#L164
.. _linaro: https://releases.linaro.org/components/toolchain/binaries/
