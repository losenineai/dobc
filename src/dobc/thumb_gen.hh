
#ifndef __thumb_gen_h__
#define __thumb_gen_h__

#include "dobc.hh"

typedef list<pcodeop *>::iterator pit;

#define R0          0
#define R1          1
#define R2          2
#define R3          3
#define R4          4
#define R5          5
#define R6          6
#define R7          7
#define R8          8
#define R9          9
#define R10         10 
#define R11         11
#define R12         12
#define SP          13
#define LR          14
#define PC          15

struct fix_item {
    int from;
    int cond;
    flowblock *to_blk;

    fix_item(int from1, flowblock *b1, int c) { from = from1; to_blk = b1; cond = c; }
};

class codegen {
    funcdata *fd;

    codegen(funcdata *f) { fd = f;  }
    ~codegen();
};

struct thumb_gen {
    funcdata *fd;
    dobc *d;
    vector<flowblock *> blist;
    vector<fix_item *> flist;

    unsigned char *data;
    int ind;

    thumb_gen(funcdata *f);
    ~thumb_gen();

    void resort_blocks();
    int run();
    void save(void);
    int run_block(flowblock *b, int b_ind);
    void add_fix_list(int ind, flowblock *b, int op);
    void topologsort();

    uint32_t reg2i(const Address &a);

    pit g_push(flowblock *b, pit pit);
    pit g_pop(flowblock *b, pit pit);
    /* 
    @ext_sp         
    我们假设有二行thumb代码:
    sub sp, sp, 4
    vpush {D8,D(}

    他们转成pcode以后，大约变成这样:

    1. sp = INT_SUB sp, 4
    2. sp = INT_SUB sp, 16
    3. mult_addr = sp
    4. store ram(mult_addr), d8
    5. mult_addr = INT_ADD mult_addr, 8
    ...

    然后在经过peephole以后，一不小心会变成这样
    1. sp = INT_SUB sp, 20
    2. mult_addr = sp

    在做代码生成的时候, pcode.1变成了一条单独的sub指令，导致原先的vpush无法生成了。
    同样的代码，在经过转换pcode以后，转换不回来了。

    所以这里每次生成sub sp指令的时候，判断后面是否有跟随d0-d16的操作，有的话，

    算出多余的 堆栈空间 (ext_sp)，传给下一个指令
    */
    pit g_vpush(flowblock *b, pit pit, int ext_sp);

    void dump();
    int dump_one_inst(int index, pcodeop *p);
};

#endif