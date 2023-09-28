fs.img: files/user_func
	cp $< $@

files/user_func: user_func.s
	@mkdir -p $(dir $@)
	nasm -f bin -o $@ $<

.PHONY: clean

clean:
	$(RM) -r fs.img files
