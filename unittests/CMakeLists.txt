#cmake_minimum_required(VERSION 3.12)

set(CMAKE_SYSTEM_NAME Android)
#set(CMAKE_ANDROID_STANDALONE_TOOLCHAIN  "/e/source/dobc_test/android-ndk-r17c-windows-x86_64/android-ndk-r17c/toolchains/llvm/prebuilt/arm-linux-androideabi-8.1/")

set(tool  "/e/source/dobc_test/android-ndk-r17c-windows-x86_64/android-ndk-r17c/toolchains/llvm/prebuilt/arm-linux-androideabi-8.1/")
set(CMAKE_C_COMPILER "${tool}/bin/clang")
set(CMAKE_CXX_COMPILER "${tool}/bin/clang++")
#set(CMAKE_ANDROID_ARM_NEON ON)


#set(OLLVM "-mllvm -fla -mllvm -sub -mllvm -bcf -mllvm -sobf -mllvm -seed=0xdeadbeef")

#add_subdirectory(hello)