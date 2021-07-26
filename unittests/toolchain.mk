
ndkroot=/e/source/dobc_test/android-ndk-r17c
#CROSS_COMPILE=/e/source/dobc_test/android-ndk-r18b/toolchains/llvm/prebuilt/arm-linux-androideabi-8.1/

ndkbuild=$(ndkroot)/ndk-build.cmd
#OLLVM_CFLAGS=-mthumb -mllvm -fla -mllvm -sub -mllvm -bcf -mllvm -sobf -mllvm -seed=0xdeadbeef $(CFLAGS)
#OLLVM_CFLAGS=-mthumb -mllvm -fla -mllvm -sub $(CFLAGS)
#OLLVM_CFLAGS=-mthumb -mllvm -bcf_prob=100 $(CFLAGS)
OLLVM_CFLAGS=-mthumb $(CFLAGS)