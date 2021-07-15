
include ../toolchain.mk

test_hello: hello.so
	$(CC) -c -o $@ test_hello.c $(CFLAGS)

hello.so : hello.c
	$(CC) -c -o $@ $< -fPIC $(OLLVM_CFLAGS) 
	cp $@ $@.decode

clean:
	rm -rf *.o *.so test_hello