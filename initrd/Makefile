DIR := files
FILE_NAMES := $(shell ls $(DIR))

FILES := $(FILE_NAMES:%=$(DIR)/%)

initrd.img: geninitrd $(FILES)
	./$< $$(for i in $(FILE_NAMES); do echo $(DIR)/$$i initrd_$$i; done)

geninitrd: geninitrd.c
	@gcc -o $@ $<

.PHONY: clean

clean:
	$(RM) -r geninitrd initrd.img
