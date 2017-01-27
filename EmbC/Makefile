#SRCS := first.c
SRCS := blink_led.c leds_test.c serial_test.c switch_test.c timer_test.c eeprom_test.c fw_app.c decode_xmodem.c
BINS := $(SRCS:.c=.bin)

all: ${BINS}

show: first_disasm

leds_test.elf: leds.o

serial_test.elf: serial.o

switch_test.elf: switch.o leds.o debug.o serial.o

timer_test.elf: timer.o leds.o debug.o serial.o

eeprom_test.elf: eeprom.o i2c.o debug.o serial.o

fw_app.elf: leds.o timer.o eeprom.o interrupt.o arm.o i2c.o debug.o serial.o

decode_xmodem.elf: serial.o debug.o

.dep: ${SRCS}
	@${CROSS_COMPILE}gcc ${CFLAGS} -MM $^ > .dep

allclean: clean
	${RM} .dep a.out

include rules.mk
-include .dep
