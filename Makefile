ASM := nasm
CC := i386-elf-gcc
LD := i386-elf-ld

VM := qemu-system-i386
GDB := gdb

SRC_DIR := src
BUILD_DIR := build

SRCS := $(shell find $(SRC_DIR) -name "*.c" -or -name "*.s")
DEPS := $(SRCS:%=$(BUILD_DIR)/%.d)

C_SRCS := $(shell find $(SRC_DIR) -name "*.c")
C_OBJS := $(C_SRCS:%=$(BUILD_DIR)/%.o)

ASMFLAGS := -I$(SRC_DIR)
CFLAGS := -Wall -Wextra -Werror -g -ffreestanding -nostdlib -MMD -MP -I$(SRC_DIR)/ -masm=intel

DISK := $(BUILD_DIR)/disk.bin
BOOT_BIN := $(BUILD_DIR)/$(SRC_DIR)/boot/main.s.bin
KERNEL_BIN := $(BUILD_DIR)/$(SRC_DIR)/kernel/kernel.bin
KERNEL_ELF := $(KERNEL_BIN:.bin=.elf)

run: $(DISK)
	$(VM) $<

debug: $(DISK) $(KERNEL_ELF)
	$(VM) -gdb tcp::1234 -S $< &
	$(GDB) -ex "target remote tcp::1234" -ex "symbol-file $(KERNEL_ELF)"

$(DISK): $(BOOT_BIN) $(KERNEL_BIN) Makefile
	@mkdir -p $(dir $@)
	dd if=/dev/zero of=$@ bs=1 count=0 seek=8192
	dd if=$(BOOT_BIN) of=$@ bs=512 seek=0 count=1 conv=notrunc
	dd if=$(KERNEL_BIN) of=$@ bs=512 seek=1 count=15 conv=notrunc

$(KERNEL_BIN): $(BUILD_DIR)/$(SRC_DIR)/boot/kernel_entry.s.o $(C_OBJS)
	@mkdir -p $(dir $@)
	$(LD) -o $@ -Ttext 0x1000 $^ --oformat binary

$(KERNEL_ELF): $(BUILD_DIR)/$(SRC_DIR)/boot/kernel_entry.s.o $(C_OBJS)
	@mkdir -p $(dir $@)
	$(LD) -o $@ -Ttext 0x1000 $^

$(BOOT_BIN): $(SRC_DIR)/boot/main.s $(KERNEL_BIN)
	@mkdir -p $(dir $@)
	$(ASM) -I$(dir $<) -f bin -MD $(BUILD_DIR)/$<.d -o $@ -DKERNEL_OFFSET=0x1000 -DKERNEL_SECTORS_SIZE=15 $<

#####################################

$(BUILD_DIR)/%.c.o: %.c
	@mkdir -p $(dir $@)
	$(CC) -c $(CFLAGS) -o $@ $<

$(BUILD_DIR)/%.s.o: %.s
	@mkdir -p $(dir $@)
	$(ASM) $(ASMFLAGS) -f elf -MD $(BUILD_DIR)/$<.d -o $@ $<

#####################################

clean:
	rm -rf $(BUILD_DIR)

.PHONY: run disk clean

-include $(DEPS)
