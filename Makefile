ASM := as
CC := gcc
LD := ld

EMU := qemu-system-x86_64
GDB := gdb

NAME := aos
SRC_DIR := src
INITRD_DIR := initrd
BUILD_DIR := build

SRCS := $(shell find $(SRC_DIR) -name "*.c" -or -name "*.S")
DEPS := $(SRCS:%=$(BUILD_DIR)/%.d)
OBJS := $(SRCS:%=$(BUILD_DIR)/%.o)

ROOT := $(BUILD_DIR)/root
ROOTLOCK := $(ROOT).lock
KERNEL := $(BUILD_DIR)/$(NAME)
INITRD := $(INITRD_DIR)/initrd.img

DISK := $(BUILD_DIR)/disk.img

BOOTDISK := $(BUILD_DIR)/bootdisk.img
BOOTCDROM := $(BUILD_DIR)/bootcdrom.iso

INC_FLAGS = -I$(SRC_DIR) -I$(SRC_DIR)/util

ASMFLAGS := -c -g
CFLAGS := -c -g \
		  -Wall -Wextra -Werror -Wno-unused-variable \
		  -fno-omit-frame-pointer -O0 \
		  -ffreestanding -fno-asynchronous-unwind-tables \
		  -mno-red-zone -mno-sse \
		  -nostdlib \
		  -MMD -MP \
		  $(INC_FLAGS)
LDFLAGS := -T linker.ld --gc-sections

EMU_FLAGS := -smp cpus=4

run_cdrom: $(BOOTCDROM)
	$(EMU) $(EMU_FLAGS) -drive file=$(BOOTCDROM),index=0,media=disk,format=raw

run_disk: $(BOOTDISK)
	$(EMU) $(EMU_FLAGS) -drive file=$(BOOTDISK),index=0,media=disk,format=raw

debug: $(BOOTCDROM) $(KERNEL)
	$(EMU) -s -S $(EMU_FLAGS) -drive file=$(BOOTCDROM),index=0,media=disk,format=raw &
	$(GDB) "$(KERNEL)" -ex "target remote tcp::1234"

disk: $(DISK)
bootcdrom: $(BOOTCDROM)
bootdisk: $(BOOTDISK)

$(DISK): $(ROOTLOCK) grub.cfg
	@mkdir -p $(dir $@)
	scripts/mkdisk $(ROOT) scripts/partitionfile $@

$(BOOTCDROM): $(ROOTLOCK)
	@mkdir -p $(dir $@)
	scripts/mkcdrom $(ROOT)/boot $@

$(BOOTDISK): $(ROOTLOCK)
	@mkdir -p $(dir $@)
	scripts/mkdisk $(ROOT) scripts/partitionfile $@ --bootable

$(ROOTLOCK): $(KERNEL) $(INITRD) grub.cfg
	@mkdir -p $(ROOT)/boot/grub
	@cp $(KERNEL) $(ROOT)/boot/$(NAME)
	@cp $(INITRD) $(ROOT)/boot/initrd.img
	@cp grub.cfg $(ROOT)/boot/grub/grub.cfg
	@touch $@

$(INITRD): .FORCE
	@$(MAKE) -C $(INITRD_DIR)

$(KERNEL): $(OBJS) linker.ld
	@mkdir -p $(dir $@)
	$(LD) $(LDFLAGS) -o $@ $(OBJS) # -flto

$(BUILD_DIR)/%.c.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -o $@ $< # -O3 -flto -ffat-lto-objects

$(BUILD_DIR)/%.S.o: %.S
	@mkdir -p $(dir $@)
	@cpp -MMD -MP $(INC_FLAGS) -o $(BUILD_DIR)/$<.S $<
	$(ASM) $(ASMFLAGS) -o $@ $(BUILD_DIR)/$<.S
	@$(RM) $(BUILD_DIR)/$<.S

#####################################

clean:
	$(RM) -r $(BUILD_DIR)
	$(MAKE) -C $(INITRD_DIR) clean

.FORCE:
.PHONY: run_cdrom run_disk debug disk bootdisk bootcdrom clean

-include $(DEPS)
