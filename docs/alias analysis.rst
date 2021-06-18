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
