CROSS_COMPILE := arm-linux-gnueabihf-
CC := ${CROSS_COMPILE}gcc
SRCS := $(wildcard *.c)
TGTS := $(SRCS:.c=)

.PHONY: all

all: ${TGTS}

thread_unsafe: thread_unsafe.c
	${CC} -g $^ -o $@ -lpthread

clean:
	${RM} ${TGTS}
