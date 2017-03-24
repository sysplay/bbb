# If called directly from the command line, invoke the kernel build system.
ifeq ($(KERNELRELEASE),)

	KERNEL_SOURCE := ../../Kernel/linux-3.12.9
	PWD := $(shell pwd)

default:
	$(MAKE) -C $(KERNEL_SOURCE) SUBDIRS=$(PWD) modules

clean:
	$(MAKE) -C $(KERNEL_SOURCE) SUBDIRS=$(PWD) clean

# Otherwise KERNELRELEASE is defined; we've been invoked from the
# kernel build system and can use its language.
else

	obj-m += first_usb_gadget_driver.o gadget_with_ep.o loopback.o
	obj-m += f_lb.o g_lb.o
	obj-m += gadget_multi.o
	obj-m += f_gpio.o g_bbb.o
	obj-m += g_bbb_multi.o
	obj-m += g_mass_storage.o g_ether.o

endif
