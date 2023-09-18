ASM := nasm
CC := i386-elf-gcc
LD := i386-elf-ld

EMU := qemu-system-i386
GDB := gdb

NAME := aos
SRC_DIR := src
BUILD_DIR := build

SRCS := $(shell find $(SRC_DIR) -name "*.c" -or -name "*.s")
DEPS := $(SRCS:%=$(BUILD_DIR)/%.d)
OBJS := $(SRCS:%=$(BUILD_DIR)/%.o)

ASMFLAGS := -g
CFLAGS := -Wall -Wextra -Werror -g -ffreestanding -nostdlib -MMD -MP -I$(SRC_DIR)/ -I$(SRC_DIR)/util -masm=intel

DISK := $(BUILD_DIR)/$(NAME).iso

KERNEL_ELF := $(BUILD_DIR)/$(NAME).elf
INITRD_IMG := $(BUILD_DIR)/initrd.img
GEN_INITRD_IMG := scripts/geninitrd

run: $(DISK)
	$(EMU) -no-reboot -no-shutdown -drive file=$<,index=0,media=disk,format=raw

debug: $(DISK) $(KERNEL_ELF)
	$(EMU) -s -S -no-reboot -no-shutdown -drive file=$<,index=0,media=disk,format=raw &
	$(GDB) -ex "target remote tcp::1234" -ex "symbol-file $(KERNEL_ELF)"

disk: $(DISK)

$(DISK): $(KERNEL_ELF) grub.cfg $(INITRD_IMG)
	@mkdir -p $(dir $@)
	@mkdir -p iso/boot/grub
	cp $(KERNEL_ELF) iso/boot/$(NAME).elf
	cp $(INITRD_IMG) iso/boot/initrd.img
	cp grub.cfg iso/boot/grub/grub.cfg
	grub-mkrescue -o $@ iso

$(INITRD_IMG): $(GEN_INITRD_IMG)
	$< initrd/hello.txt hello_initrd.txt
	mv initrd.img $@

$(GEN_INITRD_IMG): $(GEN_INITRD_IMG).c
	gcc -o $@ $<

$(KERNEL_ELF): $(OBJS) linker.ld
	@mkdir -p $(dir $@)
	$(LD) -T linker.ld -o $@ $(OBJS)

$(BUILD_DIR)/%.c.o: %.c
	@mkdir -p $(dir $@)
	$(CC) -c $(CFLAGS) -o $@ $<

$(BUILD_DIR)/%.s.o: %.s
	@mkdir -p $(dir $@)
	$(ASM) $(ASMFLAGS) -I$(dir $<) -f elf32 -MD $(BUILD_DIR)/$<.d -o $@ $<

#####################################

clean:
	rm -rf $(BUILD_DIR) iso

.PHONY: run debug disk clean

-include $(DEPS)
