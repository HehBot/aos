ASM := nasm
CC := i386-elf-gcc
LD := i386-elf-ld

EMU := qemu-system-i386
GDB := gdb

SRC_DIR := src
BUILD_DIR := build

SRCS := $(shell find $(SRC_DIR) -name "*.c" -or -name "*.s")
DEPS := $(SRCS:%=$(BUILD_DIR)/%.d)

C_SRCS := $(shell find $(SRC_DIR) -name "*.c")
C_OBJS := $(C_SRCS:%=$(BUILD_DIR)/%.o)

ASMFLAGS := -I$(SRC_DIR) -g
CFLAGS := -Wall -Wextra -Werror -g -ffreestanding -nostdlib -MMD -MP -I$(SRC_DIR)/ -I$(SRC_DIR)/util -masm=intel

DISK := $(BUILD_DIR)/disk.bin

BOOT_BIN := $(BUILD_DIR)/$(SRC_DIR)/boot/main.s.bin
INIT_PAGE_TABLE_BIN := $(BUILD_DIR)/$(SRC_DIR)/boot/init_page_table.s.bin
KERNEL_BIN := $(BUILD_DIR)/$(SRC_DIR)/kernel/kernel.bin
TEST_BIN := $(BUILD_DIR)/user/heh.s.bin

KERNEL_ELF := $(KERNEL_BIN:.bin=.elf)

run: $(DISK)
	$(EMU) -hda $<

debug: $(DISK) $(KERNEL_ELF)
	$(EMU) -s -S -hda $< &
	$(GDB) -ex "target remote tcp::1234" -ex "symbol-file $(KERNEL_ELF)"

disk: $(DISK)

$(DISK): $(BOOT_BIN) $(INIT_PAGE_TABLE_BIN) $(KERNEL_BIN) $(TEST_BIN) Makefile
	@mkdir -p $(dir $@)
	@rm -rf $@
	dd if=/dev/zero of=$@ bs=1 count=0 seek=26112 status=none
	dd if=$(BOOT_BIN) of=$@ bs=512 seek=0 count=1 conv=notrunc status=none              # sector 1
	dd if=$(INIT_PAGE_TABLE_BIN) of=$@ bs=512 seek=1 count=16 conv=notrunc status=none  # sectors 2-17
	dd if=$(KERNEL_BIN) of=$@ bs=512 seek=17 count=25 conv=notrunc status=none          # sectors 18-42
	dd if=$(TEST_BIN) of=$@ bs=512 seek=42 count=1 conv=notrunc status=none             # sector 43

$(KERNEL_BIN): $(BUILD_DIR)/$(SRC_DIR)/boot/kernel_entry.s.o $(BUILD_DIR)/$(SRC_DIR)/cpu/interrupt.s.o $(C_OBJS)
	@mkdir -p $(dir $@)
	$(LD) -o $@ -T linker.ld $^ --oformat binary

$(KERNEL_ELF): $(BUILD_DIR)/$(SRC_DIR)/boot/kernel_entry.s.o $(BUILD_DIR)/$(SRC_DIR)/cpu/interrupt.s.o $(C_OBJS)
	@mkdir -p $(dir $@)
	$(LD) -o $@ -T linker.ld $^

$(BOOT_BIN): $(SRC_DIR)/boot/main.s
	@mkdir -p $(dir $@)
	$(ASM) -I$(dir $<) -f bin -MD $(BUILD_DIR)/$<.bin.d -o $@ -DKERNEL_OFFSET=0x0 -DKERNEL_SECTORS_SIZE=25 $<

#####################################

$(BUILD_DIR)/%.c.o: %.c
	@mkdir -p $(dir $@)
	$(CC) -c $(CFLAGS) -o $@ $<

$(BUILD_DIR)/%.s.o: %.s
	@mkdir -p $(dir $@)
	$(ASM) $(ASMFLAGS) -f elf -MD $(BUILD_DIR)/$<.d -o $@ $<

$(BUILD_DIR)/%.s.bin: %.s
	@mkdir -p $(dir $@)
	$(ASM) -I$(dir $<) -f bin -MD $(BUILD_DIR)/$<.d -o $@ $<

#####################################

clean:
	rm -rf $(BUILD_DIR)

.PHONY: run disk clean

-include $(DEPS)
