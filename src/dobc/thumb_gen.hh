
#ifndef __thumb_gen_h__
#define __thumb_gen_h__

#include "dobc.hh"
#include "codegen.hh"

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

#define NG          16
#define ZR          17
#define CY          18
#define OV          19
#define BITSET_CPSR ((1 << NG) | (1 << ZR) | (1 << CY) | (1 << OV))

#define COND_EQ         0
#define COND_NE         1
#define COND_CS         2
#define COND_CC         3
#define COND_MI         4
#define COND_PL         5
#define COND_VS         6
#define COND_VC         7
#define COND_HI         8
#define COND_LS         9
#define COND_GE         10
#define COND_LT         11
#define COND_GT         12
#define COND_LE         13
#define COND_AL         14

/* A8.8 */
typedef enum thumb_inst_type {
    THUMB_ADR = 12,

    THUMB_VLD1_1 = 320,
    THUMB_VLD1_2,
    THUMB_VLD1_3,
    THUMB_VLD2_1,
    THUMB_VLD2_2,
    THUMB_VLD2_3,
    THUMB_VLD3_1,
    THUMB_VLD3_2,
    THUMB_VLD3_3,
    THUMB_VLD4_1,
    THUMB_VLD4_2,
    THUMB_VLD4_3,
    THUMB_VLDM,
    THUMB_VLDR,
} thumb_inst_type_t;

struct fix_item {
    int from;
    int cond;
    flowblock *to_blk;

    fix_item(int from1, flowblock *b1, int c) { from = from1; to_blk = b1; cond = c; }
};

class fix_inst {
public:
};

/* vldr从pc加载的指令转成pcode以后，都是4条 

vldr.64 d17,[pc,#0xe0]
1. u1 = add (pc, 4)
2. u2 = and (u1, 0xfffffffc)
3. u3 = aadd (u2, e0);
4. d17 = load [u3]

假如是 vldr.32 ，那么指令4处的output寄存器应该是 Sn
*/
struct fix_vldr_item {
    pcodeop *start;
    pcodeop *end;
    int ind;

    fix_vldr_item(pcodeop *op1, pcodeop *op2, int ind1) { start = op1; end = op2; ind = ind1; }
};

struct const_item {
    uint32_t imm = 0;
    int count = 0;
    int ind = 0;

    const_item(uint32_t i, int ind0) { imm = i; ind = ind0;  }
};

struct fix_vld1_item {
    intb loadaddr;
    int loadsiz;

    fix_vld1_item(intb loadaddr0, int loadsiz0) { 
        loadaddr = loadaddr0;
        loadsiz = loadsiz0;
    }
};


/*
ma:4 = copy r0:4
t1:4 = copy #1:4
*/

int ntz(uint32_t x);

class thumb_gen : public codegen  {
public:
    funcdata *fd;
    dobc *d;
    /* x86里面好像一条最长的变长指令是15字节，不清楚x64有没变多 */
    unsigned char fillbuf[16];

    vector<flowblock *> blist;
    vector<fix_item *> flist;
    pcodeop *curp = NULL;

    unsigned char *data = NULL;
    int ind = 0;
    int end = 0;
    int maxend = 0;
    map<uint32_t, const_item *>     constmap;

    cgtrie  cgtrie;

    thumb_gen(funcdata *f);
    ~thumb_gen();

    /*
    cfg流图需要做预处理
    1. 假设代码里面有 cmp r0, 0x12345678 这行语句，因为imm过大，导致0x12345678，
    塞不进cmp rn, imm格式中去，需要改成
    mov rn, 0x12345678
    cmp rn, rm
    这里需要空出一个寄存器的位置
    */
    void preprocess(void);
    void resort_blocks();
    int run();
    void save(void);
    int run_block(flowblock *b, int b_ind);
    void add_fix_list(int ind, flowblock *b, int op);
    void topologsort();

    uint32_t reg2i(const Address &a);

    /* 确认simd指令是否需要修复，有些simd指令，访问了pc寄存器，无法从原始obj文件中抽取 */
    bool simd_need_fix(pit it);
    /* save to so buf */
    pit retrieve_orig_inst(flowblock *b, pit pit, int save);
    /* 跟新pit到当前inst的末尾pcode上*/
    pit advance_to_inst_end(pit pit);
    pit g_push(flowblock *b, pit pit);
    pit g_pop(flowblock *b, pit pit);
    /* 
    我们假设有二行thumb代码:
    sub sp, sp, 4
    vpush {D8,D9}

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
    执行g_vpush
    */
    pit g_vpush(flowblock *b, pit pit);
    pit g_vpop(flowblock *b, pit pit);
    pit g_vstl(flowblock *b, pit pit);

    int save_to_end(uint32_t imm);

    void dump();
    int dump_one_inst(int index, pcodeop *p);
    void write_cbranch(flowblock *b, uint32_t cond);
    int regalloc(pcodeop *p);

    static uint32_t thumb_gen::stuff_const(uint32_t op, uint32_t c);
    static void stuff_const_harder(uint32_t op, uint32_t v);
    static int _add_sp_imm(int rd, int rn, uint32_t imm);
    static void _sub_sp_imm(int imm, int setflags);
    void collect_const();
    void _sub_imm(int rd, int rn, uint32_t imm, int setflags);
    void _cmp_imm(int rn, uint32_t imm);
    void fix_vldr(fix_vldr_item &item);
    void fix_vld1(fix_vld1_item &item, pc_rel_table &tab);
    /* 收集p指向的instruction中所访问的ldr位置和大小，后面坐重定位用  */
    int get_load_addr_size(pcodeop *p, intb &addr, int &size);
    int follow_by_set_cpsr(pcodeop *p1);
    void _mov_imm(int rd, uint32_t imm, int setflags, int cpsr_dead);
};

#endif