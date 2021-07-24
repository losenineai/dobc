###############
OLLVM 编译安装
###############

.. contents::
   :local:

编译环境
============
#. Window10 64bit
#. `mingw <https://telkomuniversity.dl.sourceforge.net/project/mingw-w64/Toolchains%20targetting%20Win64/Personal%20Builds/mingw-builds/8.1.0/threads-posix/seh/x86_64-8.1.0-release-posix-seh-rt_v6-rev0.7z>`_
#. `git-bash <https://github-releases.githubusercontent.com/23216272/d1c7a880-deb2-11eb-9aa0-aabc6494ff1a?X-Amz-Algorithm=AWS4-HMAC-SHA256&X-Amz-Credential=AKIAIWNJYAX4CSVEH53A%2F20210724%2Fus-east-1%2Fs3%2Faws4_request&X-Amz-Date=20210724T023416Z&X-Amz-Expires=300&X-Amz-Signature=72ce065bac393ae8649fe250f235af7433c91314986fa9db41bd774e2e857764&X-Amz-SignedHeaders=host&actor_id=618485&key_id=0&repo_id=23216272&response-content-disposition=attachment%3B%20filename%3DGit-2.32.0.2-64-bit.exe&response-content-type=application%2Foctet-stream>`_


Introduction
============
先简单说下ollvm的一些内容

#. ollvm是通过新增加一些pass来实现混淆的一个基于llvm的混淆框架
#. 一个完整的 toolchain 分为 toolchain-bin + runtimelib + header file 3部分
    #. toolchain-bin就是各种target, host组合的编译工具链
    #. runtimelib 主要就是各种运行时库
    #. header 就是各种头文件
#. ndk是一个完整的 toolchain
#. ollvm 编译出来的只是一坨 toochain-bin + lib，你最后编译出来的这坨toolchain-bin，需要让它在编译时，找到 lib 和 header，可以通过加 -L 和 -I 来指明搜索的位置

ollvm的版本，似乎要和ndk的版本对应起来，按照网上的说法:

.. list-table:: ollvm - ndk 对照
   :widths: 25 25 
   :header-rows: 1

   * - ollvm 版本
     - ndk 版本
   * - `ollvm 4.0 <https://github.com/obfuscator-llvm/obfuscator/tree/llvm-4.0>`_
     -
   * - `ollvm 6.0 <https://github.com/yazhiwang/ollvm-tll.git>`_
     - `Android ndk r17c <https://dl.google.com/android/repository/android-ndk-r17c-windows-x86_64.zip?hl=zh_cn>`_
   * - `ollvm8.0 <https://github.com/chen-yijie/obfuscator>`_
     - `Android ndk r18b <https://dl.google.com/android/repository/android-ndk-r18b-windows-x86_64.zip?hl=zh_cn>`_

工具链编译和构建
=================
执行命令:

.. code-block:: bash

    git clone https://github.com/chen-yijie/obfuscator
    mkdir build
    cd build
    cmake -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release -DCMAKE_SH="CMAKE_SH-NOTFOUND" -DCMAKE_CROSSCOMPILING=True -DLLVM_DEFAULT_TARGET_TRIPLE=arm-linux-androideabi -DLLVM_TARGET_ARCH=ARM -DLLVM_TARGETS_TO_B
    UILD=ARM  ..
    mingw32-make.exe -j4


我们讲解下CMake部分的参数

#. -G "MinGW Makefiles" : 我尝试过vs，但是记不清为什么不成功了，理论上应该可以的
#. -DCMAKE_SH="CMAKE_SH-NOTFOUND" : 必须加这个，否则会报 sh-notfound 的错误
#. -DCMAKE_CROSSCOMPILING=True : 开启交叉编译
#. -DLLVM_DEFAULT_TARGET_TRIPLE=arm-linux-androideabi : 设置默认目标3元组，这个值似乎随便设置没关系的···，只是个前缀而已，llvm似乎没有理解其语义，更加详细的说明，参考这篇文档发 `《Cross-compilation using Clang》 <https://clang.llvm.org/docs/CrossCompilation.html>`_ 
#. -DLLVM_TARGET_ARCH=ARM -DLLVM_TARGETS_TO_BUILD=ARM : 这2个参数都是设置目标环境为ARM

关于make:

#. 默认的mingw里面没有make，必须得用mingw32-make，也可以自己建立软连接 ln-s mingw32-make make
#. 后面跟的多线程参数，参考自己的核数量


编译错误解决:
--------------
实际在编译的时候，会出现这种类型的错误:

.. code-block:: make

    [ 98%] Building CXX object tools/llvm-xray/CMakeFiles/llvm-xray.dir/xray-graph.cpp.obj
    ../../lib/libLLVMObfuscation.a(Flattening.cpp.obj):Flattening.cpp:(.text+0x1e6): undefined reference to `llvm::createLowerSwitchPass()'
    collect2.exe: error: ld returned 1 exit status
    make[2]: *** [tools\lto\CMakeFiles\LTO.dir\build.make:152: bin/LTO.dll] Error 1
    make[1]: *** [CMakeFiles\Makefile2:9721: tools/lto/CMakeFiles/LTO.dir/all] Error 2


这种类型的错误，总共会出现3次，原因都是由于在CMakeLists.txt里没有link对应的Obfuscation相关的lib，我们开始改.

我们先看上文中，报错的是 tools\lto\CMakeFiles，找到tools\lto\CMakeLists.txt，原文如下:

.. code-block:: cmake

    set(LLVM_LINK_COMPONENTS
        ${LLVM_TARGETS_TO_BUILD}
        BitReader
        Core
        LTO
        MC
        MCDisassembler
        Support
        Target
        )

新增链接的库 Obfuscation 和 TransformUtils

.. code-block:: cmake

    set(LLVM_LINK_COMPONENTS
        ${LLVM_TARGETS_TO_BUILD}
        Obfuscation 
        TransformUtils
        BitReader
        Core
        LTO
        MC
        MCDisassembler
        Support
        Target
        )

重新cmake 然后 make 即可。

工具链安装
===============
编译完成以后，build下有 bin 和 lib 目录，假如你需要使用android studio来做编译，那么需要拷贝到对应的 ndk的 toolchains/llvm/prebuilt 目录下，但是假如你打算裸用命令行，其实拷贝到哪里都无所谓，主要是需要用 -L 指明 lib 搜索的路径，然后 -I指明要搜索的头文件位置，具体的可以参考 ``unittests\toolchain.mk``

ollvm编译目标文件
==================
这个网络上太多了，我就不细说了

不要使用seed?
-------------
这个参数是否无效，而且会导致一些奇怪的问题

ollvm编译目标文件碰到的问题
==============================
实际在使用的时候，发现了一些问题，需要记录以下

如何只编译目标函数
-------------------------------
参考官方 wiki: `《Functions annotations》 <https://github.com/obfuscator-llvm/obfuscator/wiki/Functions-annotations>`_，

假如需要加多个的话，就这样加:

``__attribute((__annotate__(("bcf,fla,sub"))))``

编译目标函数不生效
-------------------------------
实际测试，经常发现，这个混淆没有效果，开了和没开一样，似乎有一定概率

如何控制随机
-------------------------------
能否指定随机因子的情况下，然后让每次生成的文件都一样。