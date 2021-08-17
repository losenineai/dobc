
#ifndef __high_h___
#define __high_h___

#include "varnode.hh"

class flowblock;

/* 这个头文件是为了把低级的由cv, ov, zr, ng组合成的比较指令，转换成高级的比较指令 */

/* 这里的条件，我是直接参考的《arm_arch_ref》 A8.3 但是我没有严格搞清一些条件的意义 

=========================================================================================================
cond    Mnemonic    Meaning (integer)               Meaning (floating-point)^a          Condition flags
        extension 
=========================================================================================================
0000    EQ          Equal                           Equal                               Z == 1
=========================================================================================================
0001    NE          Not equal                       Not equal, or unordered             Z == 0
=========================================================================================================
0010    CS^b        Carry set                       Greater than, equal, or unordered   C == 1
=========================================================================================================
0011    CC^c        Carry clear                     Less than                           C == 0
=========================================================================================================
0100    MI          Minus, negative                 Less than                           N == 1
=========================================================================================================
0101    PL          Plus, positive or zero          Greater than, equal, or unordered   N == 0
=========================================================================================================
0110    VS          Overflow                        Unordered                           V == 1
=========================================================================================================
0111    VC          No overflow                     Not unordered                       V == 0
=========================================================================================================
1000    HI          Unsigned higher                 Greater than, or unordered          C == 1 and Z == 0
=========================================================================================================
1001    LS          Unsigned lower or same          Less than or equal                  C == 0 or Z == 1
=========================================================================================================
1010    GE          Signed greater than or equal    Greater than or equal               N == V
=========================================================================================================
1011    LT          Signed less than                Less than, or unordered             N != V
=========================================================================================================
1100    GT          Signed greater than             Greater than                        Z == 0 and N == V
=========================================================================================================
1101    LE          Signed less than or equal       Less than, equal, or unordered      Z == 1 or N != V
=========================================================================================================
1110    None (AL)^d Always (unconditional)          Always (unconditional)              Any
=========================================================================================================
a. Unordered means at least one NaN operand.
b. HS (unsigned higher or same) is a synonym for CS.
c. LO (unsigned lower) is a synonym for CC.
d. AL is an optional mnemonic extension for always, except in IT instructions. For details see IT on page A8-390.
=========================================================================================================


我现在在使用的有
1. EQ, NE, 相等和不等，这个比较容易理解
2. GT, LE, 有符号的 大于和小于等于
3. GE, LT, 有符号的 大于等于和小于
4. HI, LS, 无符号的 大于和小于等于
5. CS, CC, 无符号的 大于等于和小于 

没有使用的
1. AL AL一般不会出现在带cbranch指令中，有的话，通过优化干掉
2. MI, PL, VS, VC，我看的不是特别懂，它们有带unordered，文档里说，这代表其中至少一个数是NaN
*/
enum high_cond_type {
    h_eq = 0,
    h_ne = 1,
    h_cs = 2,
    h_cc = 3,
    h_mi = 4,
    h_pl = 5,
    h_vs = 6,
    h_vc = 7,
    h_hi = 8,
    h_ls = 9,
    h_ge = 10,
    h_lt = 11,
    h_gt = 12,
    h_le = 13,
    h_al = 14,
    h_unkown = 15
};

class high_cond {

public:
    enum high_cond_type type = h_unkown;
    int version;

    flowblock *from;
    flowblock *to;

    varnode *lhs;
    varnode *rhs;

    /* 很多的条件具有等价性 
    x.0 = 1234;
    z.0 = 0;
    if (y > 10)
        z.1 = 1;
    z.2 = phi(z.0, z.1)
    if (z.2)
        x.1 = 2345;
    x.2 = phi(x.0, x.1)

    在这里 y > 10 等于 z.1 == 1，我们认为这2个条件具有传递性

    */
    high_cond *link;

    high_cond();
    ~high_cond();

    enum high_cond_type compute_cond(flowblock *b);
    int update(flowblock *t);
    int update(flowblock *t, flowblock *b);
    /*

    @return     0   success
                -1  error
    */
    int linkto(high_cond &op2);
    enum high_cond_type  get_type(void);
    high_cond &operator=(const high_cond &op2);
    bool operator==(const high_cond &op2) const;
    bool operator!=(const high_cond &op2) const;
    high_cond &not(const high_cond &op2);
};

inline high_cond &high_cond::operator=(const high_cond &op2)
{
    from = op2.from;
    to = op2.to;
    type = op2.type;
    lhs = op2.lhs;
    rhs = op2.rhs;
    link = op2.link;

    return *this;
}

inline bool high_cond::operator==(const high_cond &op2) const
{
    const high_cond *node = &op2;

    for (; node; node = node->link) {
        if (!lhs->is_constant() && !node->lhs->is_constant() && (lhs == node->lhs)) {
            if (rhs->is_constant() && lhs->is_constant() && rhs->type == node->rhs->type)
                return true;
        }
    }

    return false;
}

inline bool high_cond::operator!=(const high_cond &op2) const
{
    return !(*this == op2);
}

#endif
