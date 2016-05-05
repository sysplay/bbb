DIRS := $(filter-out Makefile module.mk, $(wildcard *))
DIRS_CLEAN := $(DIRS:=_clean)

.PHONY: all ${DIRS} clean allclean

all: ${DIRS}

${DIRS}:
	@echo -n "Building $@ ... "
	@${MAKE} -C $@ > /dev/null
	@echo "done"

clean: ${DIRS_CLEAN}

%_clean:
	@echo -n "Cleaning $* ... "
	@${MAKE} -C $* clean > /dev/null
	@echo "done"

allclean: clean
	@${RM} *.sh
