ASM := nasm
CC := i386-elf-gcc
LD := i386-elf-ld

VM := qemu-system-i386
GDB := gdb

SRC_DIR := src
BUILD_DIR := build

SRCS := $(shell find $(SRC_DIR) -name "*.c" -or -name "*.s")
DEPS := $(SRCS:%=$(BUILD_DIR)/%.d)

ASMFLAGS := -I$(SRC_DIR)/

DISK := $(BUILD_DIR)/disk.bin
BOOT_BIN := $(BUILD_DIR)/$(SRC_DIR)/boot/main.s.bin

run: $(DISK)
	$(VM) $<

disk: $(DISK)

$(DISK): $(BOOT_BIN)
	@mkdir -p $(dir $@)
	cat $^ > $@

#####################################

$(BUILD_DIR)/%.c.o: %.c
	@mkdir -p $(dir $@)
	$(CC) -c $(CFLAGS) -o $@ $<

$(BUILD_DIR)/%.s.bin: %.s
	@mkdir -p $(dir $@)
	$(ASM) $(ASMFLAGS) -f bin -MD $(BUILD_DIR)/$<.d -o $@ $<

$(BUILD_DIR)/%.s.o: %.s
	@mkdir -p $(dir $@)
	$(ASM) $(ASMFLAGS) -f elf -MD $(BUILD_DIR)/$<.d -o $@ $<

#####################################

clean:
	rm -rf $(BUILD_DIR)

.PHONY: run disk clean

-include $(DEPS)
