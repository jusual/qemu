CC = arm-none-eabi-gcc
OBJCOPY = arm-none-eabi-objcopy

SOURCES = $(wildcard *.S)
BINARIES = $(patsubst %.S,%.hex,$(SOURCES))

.PHONY: clean

all: $(BINARIES)

clean:
	rm -f $(BINARIES) *.o

%.o: %.S %.ld
	$(CC) -nostdlib -Wl,--build-id=none -O0 -x assembler-with-cpp -T $(patsubst %.S,%.ld,$<) -x assembler-with-cpp -o $@ $<

%.hex: %.o
	$(OBJCOPY) -O ihex $< $@

run-test-armv6m-undef: test-armv6m-undef.hex
	qemu-system-arm -nographic -semihosting -M microbit -kernel $<
