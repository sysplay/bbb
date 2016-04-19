CROSS_COMPILE = arm-linux-gnueabihf-

CC = ${CROSS_COMPILE}gcc
AS = ${CROSS_COMPILE}as
LD = ${CROSS_COMPILE}ld
OBJCOPY = ${CROSS_COMPILE}objcopy
OBJDUMP = ${CROSS_COMPILE}objdump

#OPTIMIZATION_FLAGS = -O2
CFLAGS = -mcpu=cortex-a8 ${OPTIMIZATION_FLAGS}
#CFLAGS = -mcpu=cortex-a8 -mfpu=neon ${OPTIMIZATION_FLAGS}
ASFLAGS = -mcpu=cortex-a8
#ASFLAGS = -mcpu=cortex-a8 -mfpu=neon
# TODO
#LDFLAGS = -T first.lds
#LDFLAGS = -T linker_first.lds
LDFLAGS = -T linker.lds

TARGET ?= code

default: ${TARGET}.bin

%.bin: %.elf
	${OBJCOPY} --gap-fill=0xFF -O binary $< $@

# TODO
#%.elf: %.o
#%.elf: %.o first_startup.o
#%.elf: %.o common.o startup_first.o
%.elf: %.o common.o startup.o interrupt.o
	${LD} ${LDFLAGS} $^ -o $@

#%.o: %.c
#	${CC} ${CFLAGS} -c $< -o $@
#
#%.o: %.s
#	${AS} ${ASFLAGS} -c $< -o $@
#
view: ${TARGET}_disasm

%_disasm: %.elf
	${OBJDUMP} -d $<

clean:
	${RM} *.bin *.elf *.o
