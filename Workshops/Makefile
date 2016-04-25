WORKSHOPS := ECFD
READY_WORKSHOPS := $(shell echo ${WORKSHOPS} | tr [:upper:] [:lower:])
REVERT_WORKSHOPS := $(READY_WORKSHOPS:=_revert)

.PHONY: all revert

all: ${READY_WORKSHOPS}

revert: ${REVERT_WORKSHOPS}

%: %.patch
	patch -d .. -p0 < $< >> /dev/null

%_revert: %.patch
	patch -d .. -R -p0 < $< >> /dev/null
