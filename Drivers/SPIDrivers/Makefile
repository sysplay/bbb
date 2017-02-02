# If called directly from the command line, invoke the kernel build system.
ifeq ($(KERNELRELEASE),)

	KERNEL_SOURCE := ../../Kernel/linux-3.12.9
	PWD := $(shell pwd)
default:
	$(MAKE) -C $(KERNEL_SOURCE) SUBDIRS=$(PWD) modules ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- CFLAGS=-DDEBUG

clean:
	$(MAKE) -C $(KERNEL_SOURCE) SUBDIRS=$(PWD) clean

# Otherwise KERNELRELEASE is defined; we've been invoked from the
# kernel build system and can use its language.
else

	#src-m := $(notdir $(wildcard ${SUBDIRS}/*.c))
	#src-mod := $(notdir $(wildcard ${SUBDIRS}/*.mod.c))
	#src-m := $(filter-out ${src-mod}, ${src-m})
	#obj-m := $(src-m:.c=.o)
	obj-m := spi_test.o spi1.o spi2.o spi.o
        spi_test-y := spi_char.o test.o
        spi1-y := spi_char.o s1.o
        spi2-y := spi_char2.o s1.o

endif
