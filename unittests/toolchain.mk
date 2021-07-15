
CROSS_COMPILE=/e/source/dobc_test/android-ndk-r17c-windows-x86_64/android-ndk-r17c/toolchains/llvm/prebuilt/arm-linux-androideabi-8.1/

CC=$(CROSS_COMPILE)/bin/clang
CFLAGS=-I$(CROSS_COMPILE)/sysroot/usr/include/arm-linux-androideabi -fstack-protector-all
OLLVM_CFLAGS=-mthumb -mllvm -fla -mllvm -sub -mllvm -bcf -mllvm -sobf -mllvm -seed=0xdeadbeef $(CFLAGS)
