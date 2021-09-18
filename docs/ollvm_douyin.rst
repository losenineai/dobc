###############
OLLVM 抖音变形
###############

.. contents::
   :local:

分析
============
抖音的ollvm和传统的ollvm，有以下区别

#. cfg做了切割，导致传统的反编译器无法正常工作
#. sp不平衡，导致很难正确判断函数是否完整

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


函数inline
============

样本
============
data/抖音/v17.3