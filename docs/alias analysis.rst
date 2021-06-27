#############
别名分析
#############

.. contents::
   :local:

别名空间:
==========
别名其实就是内存引用，一般在Ghdira里面，用load, store访问，一般的别名空间分为多种:

* stack virtual memory: 局部变量
* heap: 运行时生成的堆
* argument space: 参数本身是个指针
* bss: bss段，静态变量，全局变量访问的位置
* rodata: 只读数据
* text: 只读代码段

对不同的数据段访问策略会有一些区别

stack
-------
现在Ghidra里面使用了一个叫 SpacebaseSpace的空间来做这个行为，但是设计的数据结构有点问题，这种 基址空间 ，在设计中，我感觉它是绑定倒某个寄存器得。


Ghidra实现
^^^^^^^^^^
在具体得实现中，有个问题我没理解。假设，sp - 4，在Ghidra中表示为

.. code-block:: c

	Address.AddrSpace = StackbaseSpace
	Address.offset = (uintb)-8 	==> Addrss.offset = 0xffffffffffffff8


但是在我们使用heritage进行搜索哪些地方使用了这个值时，会加一个size

.. code-block:: c

	Address &addr = vn->getAddr();
	addr += 4 ==> Address.offset = 0x00000000fffffffc

这样加过以后，会导致符号对不上了，整个语义也发生了变化如何处理？

我现在在代码中，直接把所有得StackbaseSpace里得变量，加了一个偏移（又是模拟得思路）

.. code-block:: c

	#define STACK_BASE_OFFSET 		0x100000
	Address addr(StackbaseSpace, -8 + STACK_BASE_OFFSET);
	varnode *vn = create_vn(addr, size)

heap
-------
space[heap] ∩ space[stack] = ∅
heap(n) != heap(m)  ==> space[heap(n)] ∩ space[heap(m)] = ∅

运行时的堆，虽然它的值属于T，但是它和virtual_stack是正交的。所以别名可以穿透它

.. code-block:: c

	1. store [sp + 4], r4
	2. r0 = call malloc
	3. store [r0 + 4], 1
	4. r1 = load [sp + 4]

r0 == T -> r0+4 = T

理论上指令4，往上搜索，无法关联指令1，因为 store[T] 会导致may def，但是因为heap和stack正交，所以是可以穿透这个指令得

argument space
----------------
space[argument] ∩ space[stack-] = ∅
space[argument] ∩ space[stack+] = T


bss
----------------
.bss[offset] == T
bss段读出来的值，都要当T处理，static类型的你要可以识别出来，在入口出可以为一次0

rodata
---------------
.rodata[offset] = C

text
---------------
.text[offset] = C


数据结构
============
别名其实就是内存引用，一般在Ghdira里面，用load, store访问，一般的


算法分析
========

传统的算法
----------

dobc的别名分析难点在于，我们对函数和数据的类型识别不够，导致在做别名传播时，容易引入大量的maydef, mayuse，从而造成效率低下。

假设我们无法识别出函数类型，那么在一个函数中有n个call, callind时，m个正交的store, k个正交的load，那么总计会插入: 

1. n * m 个maydef
2. n 个 mayuse

这里mayuse的数量和call是1比1，因为mayuse可以接很多个in。

假设你可以识别出某个函数类型是:

.. code-block:: c

	void free(void *);

那么因为我们知道free不会修改space(stack)上的内容，所以我们去掉所有的maydef，又因为它的参数只有一个，假如这个时候r0的计算是具体的某个space(stack)或者heap(stack)上的位置，那么我们也可以精确的去掉所有的mayuse

在libkwsgmain.so:sub_cb59中大约有200个call，10+ stop tore, 230正交的sp-store， 假如全部插入maydef，大约要引入5W左右节点，在我们还没有能力提供更好的函数类型和数据类型分析时，我们可能不会考虑引入这种算法

传统算法的细节的内容可以参考:`HSSA <https://citeseerx.ist.psu.edu/viewdoc/download?doi=10.1.1.33.6974&rep=rep1&type=pdf>`_.

新的算法
-----------

优点和缺点:
------------

性能?
^^^^^^
因为我没有完整的实现过传统的mem-phi算法，所以我也不确认是否哪个更好一些

更强的表达能力？
^^^^^^^^^^^^^^^^^
没有识别出更精确的别名信息，识别出的信息反而减少了

算法更简单？
^^^^^^^^^^^^^^^^^
原理上是的，不用大量的插入zero-version

那为什么要用新的算法呢？
^^^^^^^^^^^^^^^^^^^^^^^^
因为不用插入maydef, mayuse

新算法最大的问题
^^^^^^^^^^^^^^^^^^^^^^^
实际上新的算法退回到了老的数据流分析框架中的算法了，而不是基于现在的du链。这导致了新的基于ssa du链的一些机制无法完全在这套别名框架中复用了。


算法实现:
----------
我们把别名收集

在第一次 heritage 以后，对于任意的store