
include ../toolchain.mk

hello.so : hello.c
	$(CC) -c -o $@ $< -fPIC $(OLLVM_CFLAGS) 

clean:
	rm -rf *.o *.so