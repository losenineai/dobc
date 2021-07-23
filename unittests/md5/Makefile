
include ../toolchain.mk

md5.so : md5.c
	$(CC) -c -o $@ $< -fPIC $(OLLVM_CFLAGS) 
	cp $@ $@.decode

.PHONY: clean
clean:
	rm -rf *.o *.so 
