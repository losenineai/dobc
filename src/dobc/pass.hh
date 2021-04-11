
#ifndef __pass_hh__
#define __pass_hh__

#include "sleigh.hh"
#include "dobc.hh"


/*
所有的pass
return
<0: 内部出错
0:  没做任何改动退出
>0 .  有做改动，具体什么改动，看具体的值
*/

#define PASS_ERROR(x)               (x < 0)
#define PASS_DO_NOTHING(x)          (x == 0)
#define PASS_DO_STHING(x)           (x > 0)

class pass {
public:
    funcdata *fd;
    dobc *d;

    pass(funcdata *f) { fd = f; d = fd->d; }
};

/* 条件缩减:

r0 = call.func
r1 = 0x50501234
if (r0) r1 = 0x34567899
...
...
if (r1 == 0x50501234) {
}
else {
}

改成
r0 = call.func
r1 = r0;

if (r1) {
}
else {
}

note:上面的r1的数字是乱写的
*/
class pass_cond_reduce : public pass {
public:
    pass_cond_reduce(funcdata *f):pass(f) {}
    int run(void);
};

/*
arm不支持在opcode 的立即数中支持任意大小的数，比如:

cmp r0, 0x12345678

它有一些填充数的规则，但是假如这些规则不生效时，就怎么都填充不进去了，具体可以参考:

thumb:  aar.A6.3.2
arm  :  aar.A5.2.4

当我们填充不进去时，就必须把上文中的形式改成如下：

cpy rn, 0x12345678
cmp r0, rn

这里必须得做寄存器分配找到一个可以用的寄存器rn

我们对这种特殊指令进行寄存器分配，其余的都不动，假如没有找到合适的寄存器，有2个方案:

1. 重新对全局做寄存器分配
2. 直接退出，报错

这里采用了第2种方案，因为ollvm浪费了不少寄存器，化简以后，我个人感觉一定是可以找到的，
这个涉及到对ollvm的理解:

TODO:理解部分

*/
class pass_regalloc_const_arm : public pass {
public:
    pass_regalloc_const_arm(funcdata *f):pass(f) {  }
    int run(void);
};

class pass_mgr {
};

#endif