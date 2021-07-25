
include ../toolchain.mk

md5.so : md5.c
	$(CC) -shared -undefined dynamic_lookup $(OLLVM_CFLAGS) -o $@ $< 
	cp $@ $@.decode

.PHONY: clean
clean:
	rm -rf *.o *.so 
