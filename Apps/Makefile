CROSS_COMPILE := arm-linux-gnueabihf-
CC := ${CROSS_COMPILE}gcc
SRCS := $(wildcard *.c)
TGTS := $(SRCS:.c=)

.PHONY: all

all: ${TGTS}

clean:
	${RM} ${TGTS}
