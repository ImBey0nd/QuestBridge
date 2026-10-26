ifndef PS5_PAYLOAD_SDK
$(error Set PS5_PAYLOAD_SDK to an installed ps5-payload-dev SDK)
endif
include $(PS5_PAYLOAD_SDK)/toolchain/prospero.mk
.DEFAULT_GOAL := all
all: questbridge-probe.elf
questbridge-probe.elf: main.c
	@echo "Build-only probe: pinned SDK startup patches kernel state before main; see docs/research/PS5_FEASIBILITY.md."
	$(CC) -Wall -Wextra -Werror -o $@ $<
# Deliberately no deployment or exploit target.
