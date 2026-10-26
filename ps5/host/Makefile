ifndef PS5_PAYLOAD_SDK
$(error Set PS5_PAYLOAD_SDK to an installed ps5-payload-dev SDK)
endif
include $(PS5_PAYLOAD_SDK)/toolchain/prospero.mk
all: questbridge-probe.elf
questbridge-probe.elf: main.c
	$(CC) -Wall -Wextra -Werror -o $@ $<
# Deliberately no deployment or exploit target.
