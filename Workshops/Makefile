WORKSHOPS := ECFD
READY_WORKSHOPS := $(shell echo ${WORKSHOPS} | tr [:upper:] [:lower:])
REVERT_WORKSHOPS := $(READY_WORKSHOPS:=_revert)

.PHONY: all revert

all: ${READY_WORKSHOPS}

revert: ${REVERT_WORKSHOPS}

%: %.patch
	@echo -n "$* " | tr 'a-z' 'A-Z'
	@patch -d .. -r - -p0 < $< >> /dev/null || echo -n "already "
	@echo "ready"

%_revert: %.patch
	@echo -n "$* " | tr 'a-z' 'A-Z'
	@patch -d .. -r - -R -p0 < $< >> /dev/null || echo -n "already "
	@echo "reverted"
