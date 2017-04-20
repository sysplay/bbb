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
	obj-m := serial-test.o serial1.o serial2.o serial3.o tty_serial.o tiny_serial.o
        serial-test-y := char_serial.o test.o
        serial1-y := char_serial.o s1.o
        serial2-y := char_serial.o s2.o
        serial3-y := char_serial.o s3.o


endif
