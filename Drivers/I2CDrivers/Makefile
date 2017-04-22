# If called directly from the command line, invoke the kernel build system.
ifeq ($(KERNELRELEASE),)

	KERNEL_SOURCE := ../../Kernel/linux-3.12.9/
	PWD := $(shell pwd)
default:
	$(MAKE) -C $(KERNEL_SOURCE) SUBDIRS=$(PWD) modules ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- CFLAGS=-DDEBUG

clean:
	$(MAKE) -C $(KERNEL_SOURCE) SUBDIRS=$(PWD) clean

# Otherwise KERNELRELEASE is defined; we've been invoked from the
# kernel build system and can use its language.
else

	obj-m := i2c_test.o i2c1.o i2c2.o i2c3.o i2c4.o i2c_client_adap.o i2c_client.o i2c_adap.o at24.o i2c_intr.o
	i2c_test-y := i2c_char.o i2c-omap.o test1.o
	i2c1-y := i2c_char.o i2c-omap.o s1.o
	i2c2-y := i2c_char.o i2c-omap.o s2.o
	i2c3-y := i2c_char.o i2c-omap.o s3.o
	i2c4-y := i2c_char4.o i2c-omap.o s4.o
	i2c_intr-y := i2c_char_intr.o s_intr.o i2c-omap_intr.o 
	i2c_client_adap-y := i2c_char_client.o i2c_adap.o

endif
