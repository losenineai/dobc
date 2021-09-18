###############
OLLVM 抖音变形
###############

.. contents::
   :local:

分析
============
抖音的ollvm和传统的ollvm，有以下区别

#. cfg-split，导致传统的反编译器无法正常工作
#. sp-imbalance，导致很难正确判断函数是否完整
#. function-split, 把一个ollvm函数拆成多个函数

这里最重要的是判断函数完整性，然后何时做优化。

判断函数完整性
==============
我们要在一个完整的函数上做分析，做de-ollvm才能得到我们想要的结果，假如拿到的cfg都是残缺不全的，
那么在这个上面做优化得到的结果也不对。

#. 假如函数没有对sp的操作，我们认为它天然平衡，即本身就是一个完整的函数
#. 假如函数有对sp的操作，而且sp没有被挪作临时寄存器（这一点不好判断），那么当存在以下几个条件我们认为cfg完整
    #. 存在着某条路径，到非 _stack_check_fail 的 exit-node 出口平衡
    #. 在这条路径上的最后一个导致堆栈平衡的指令，必须得在 exit-node 的支配节点路径上

上面这个只代表到今天(2021年9月18日)我的想法，随着攻防的对抗，可能需要不断的完善各种corner-case，来加强判断的正确性


实现调整:
==========
以前dobc的follow_flow的流程非常简单，如下

.. code:: c

    /* 伪代码 */

    function generate_blocks() {
        collect_edge();
        split_basic();
        connect_basic();
    }

    generate_ops();
    generate_blocks();
    heritage();
    deollvm();

是一个线性的过程，现在我们调整为

.. code:: c


    while (is_complete_function()) {
        generate_ops();
        generate_blocks();
        heritage();
    }
    deollvm();

这里有个heritage的过程，其实就是一个pass集合，里面主要做了ssa-reconstruction，因为假如不做分析，
不做常量传播，我们可能无法知道某些 temp exit-node 的下一跳地址在何处

注意事项
------------

#. 我们的代码在对cfg反复重建的过程中，一定要彻底的关闭会修改pcodeop的行为，因为cfg重建以后，很多out varnode的值可能会发生改变


function-split
===============

如下所示:

.. figure:: imgs/empty.jpg
   :alt:

函数的切割，只有一种方法可以解决，就是函数inline，但是具体如何inline有几个问题需要处理

how to detect which function need be inline?
----------------------------------------------
很难

#. pattern
#. 手工标注

inline function lead cfg node extend exponentially
------------------------------------------------------
假如你直接把函数inline进来，会让整个程序膨胀的非常厉害，进而拖累整个分析的速度，现在采取
的方法是带着参数去inline，比如以下:

.. code:: c

    int mathop(int op, int a, int b)
    {
        switch (op) {
            case 0: return a + b;
            case 1: return a - b;
            case 2: return a * b;
            case 3: return a / b;
            case 4: return a % b;
        }
    }

    int sign()
    {
        int c  = mathop(0, 1, 2);
        int c1 = mathop(1, 1, 2);

        return  mathop(2, c, c1);
    }

假如先inline函数，会变成这样:

.. code:: c

    int sign()
    {
        int ret, op = 0, a = 1, b 2;
        switch (op) {
            case 0: ret =  a + b; break;
            case 1: ret =  a - b; break
            case 2: ret = a * b; break;
            case 3: ret = a / b; break;
            case 4: ret = a % b; break;
        }
        int c = ret;
        op = 1, a = 1, b = 2;
        switch (op) {
            case 0: ret =  a + b; break;
            case 1: ret =  a - b; break
            case 2: ret = a * b; break;
            case 3: ret = a / b; break;
            case 4: ret = a % b; break;
        }
        int c1 = ret;

        op = 2, a = c, b = c1;
        switch (op) {
            case 0: ret =  a + b; break;
            case 1: ret =  a - b; break
            case 2: ret = a * b; break;
            case 3: ret = a / b; break;
            case 4: ret = a % b; break;
        }
        return  ret;
    }

这样的好处是很方便，坏处是cfg很容易膨胀的不可控制，所以我们调整策略，把参数传入函数优化
完以后，在送进来inline，好处是体积更小，坏处是不好实现，但是在体积大小的控制上会更好。
举例：

.. code:: c

    int sign()
    {
        int c  = 3;
        int c1 = -1;

        return  a * b;
    }


参数inline带来的严重弊端
--------------------------
你inline的结果可能是错的，因为我们是一边修复cfg，一边inline，那么可能等你修复完cfg以
后，发现某条cfg的边指向了了你inline的那个函数所在的cfgA，而这可能会导致inline的结果发
生变化。


现在这种情况我还没有处理，可以在所有可以connect 到 cfgA 的 cfg 上打标记，假如出现新边
直接报错。需要实际测试一下是否有这种情况





样本
============
data/抖音/v17.3