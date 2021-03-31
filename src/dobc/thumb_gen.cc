
#include "sleigh.hh"
#include "thumb_gen.hh"
#include <assert.h>
#include "vm.h"

#define pi0(p)              p->get_in(0)
#define pi1(p)              p->get_in(1)
#define pi1(p)              p->get_in(1)
#define pi2(p)              p->get_in(2)
#define pi0a(p)             p->get_in(0)->get_addr()
#define pi1a(p)             p->get_in(1)->get_addr()
#define pi2a(p)             p->get_in(2)->get_addr()
#define poa(p)              p->output->get_addr()
#define a(vn)               ((vn)->get_addr())
#define ama                 d->ma_addr
#define asp                 d->sp_addr
#define ar0                 d->r0_addr
#define apc                 d->pc_addr
#define alr                 d->lr_addr
#define azr                 d->zr_addr
#define istemp(vn)          ((vn)->get_addr().getSpace()->getType() == IPTR_INTERNAL)
#define isreg(vn)           d->is_cpu_base_reg(vn->get_addr())

#define ANDNEQ(r1, r2)      ((r1 & ~r2) == 0)
#define ANDEQ(r1, r2)      ((r1 & r2) == r2)
#define in_imm3(a)          ANDNEQ(a, 0x07)
#define in_imm4(a)          ANDNEQ(a, 0x0f)
#define in_imm5(a)          ANDNEQ(a, 0x1f)
#define in_imm7(a)          ANDNEQ(a, 0x7f)
#define in_imm8(a)          ANDNEQ(a, 0xff)
#define align4(a)           ((a & 3) == 0)

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

/* A8.8.119 */
#define NOP1            0xbf00
#define NOP2            0xf3af8000

#define imm_map(imm, l, bw, r)          (((imm >> l) & ((1 <<  bw) - 1)) << r)
#define bit_get(imm, l, bw)             ((imm >> l) & ((1 << bw) - 1))


#define read_thumb2(b)          ((((uint32_t)(b)[0]) << 16) | (((uint32_t)(b)[1]) << 24) | ((uint32_t)(b)[2]) | (((uint32_t)(b)[3]) << 8))
#define write_thumb2(i, buf) do { \
            (buf)[2] = i & 255; \
            i >>= 8; \
            (buf)[3] = i & 255; \
            i >>= 8; \
            (buf)[0] = i & 255; \
            i >>= 8; \
            (buf)[1] = i & 255; \
            i >>= 8; \
    } while (0)


uint32_t bitcount(uint32_t x)
{
    x = (x & 0x55555555) + ((x >> 1) & 0x55555555);
    x = (x & 0x33333333) + ((x >> 2) & 0x33333333);
    x = (x & 0x0F0F0F0F) + ((x >> 4) & 0x0F0F0F0F);
    x = (x & 0x00FF00FF) + ((x >> 8) & 0x00FF00FF);
    x = (x & 0x0000FFFF) + ((x >>16) & 0x0000FFFF);

    return x;
}

static  thumb_gen *g_cg = NULL;

thumb_gen::thumb_gen(funcdata *f)
{
    fd = f;
    d = fd->d;
    data = f->bufptr;
    g_cg = this;
}

thumb_gen::~thumb_gen()
{
}

void thumb_gen::resort_blocks()
{
    blist = fd->bblocks.blist;
}

#define UNPREDICITABLE()        throw LowlevelError("not support");

uint32_t thumb_gen::reg2index(const Address &a)
{
    return (a.getOffset() - ar0.getOffset()) / 4;
}

int     thumb_gen::calc_code_size(flowblock *b)
{
    return 0;
}

static void ot(uint16_t i)
{
    g_cg->data[g_cg->ind++] = i & 255;
    i >>= 8;
    g_cg->data[g_cg->ind++] = i & 255;
}

static void ott(uint32_t i)
{
    write_thumb2(i, g_cg->data + g_cg->ind);
    g_cg->ind += 4;
}

static void o(uint32_t i)
{
    if ((i >> 16))
        ott(i);
    else
        ot((uint16_t)i);
}

static uint32_t stuff_const(uint32_t op, uint32_t c)
{
    int try_neg = 0;
    uint32_t nc = 0, negop = 0;

    switch (op & 0x1f00000) {
    case 0x1000000:     // add
    case 0x1a00000:     // sub
        try_neg = 1;
        negop = op ^ 0xa00000;
        nc = -c;
        break;
    case 0x0400000:     // mov or eq
    case 0x0600000:     // mvn
        if (ANDEQ(op, 0xf0000)) { // mov
            try_neg = 1;
            negop = op ^ 0x200000;
            nc = ~c;
        }
        else if (c == ~0) { // orr
            return (op & 0xfff0ffff) | 0x1E00000;
        }
        break;
    case 0x800000:      // xor
        if (c == ~0)
            return (op & 0xe0100f00) | ((op >> 16) & 0xf) | 0x4f0000;
        break;
    case 0:             // and
        if (c == ~0)
            return (op & 0xe0100f00) | ((op >> 16) & 0xf) | 0x6f0000;
    case 0x0200000:     // bic
        try_neg = 1;
        negop = op ^ 0x200000;
        nc = ~c;
        break;
    }

    do {
        /* A6.3.2 */
        uint32_t m;
        int i, c1, c2;
        if (c < 256)
            return op | c;
        c1 = c & 0xff;
        if ((c2 = (c1 | (c1 << 16))) == c)
            return op | c | 0x0100;
        if ((c2 << 8) == c)
            return op | c | 0x0200;
        if ((c1 | c2) == c)
            return op | c | 0x0300;
        for (i = 8; i < 32; i ++) {
            m = 0xff << (32 - i);
            if (!(c & ~m) && (c && (1 << (39 - i))))
                return op | ((i & 0x10) << 22) | ((i & 0xe) << 11) | ((i & 1) << 7) | ((c) >> (32 - i));
        }
        op = negop;
        c = nc;
    } while (try_neg--);

    return 0;
}

static uint32_t stuff_constw(uint32_t op, uint32_t c, int m)
{
    if (c >= (1 << m)) return 0;

    op |= imm_map(c, 11, 1, 26) | imm_map(c, 8, 3, 12) | imm_map(c, 0, 8, 0);
    if (m == 16) op |= imm_map(c, 12, 4, 16);
    return op;
}

static void stuff_const_harder(uint32_t op, uint32_t v)
{
    uint32_t x;
    x = stuff_const(op, v);
    if (x)
        o(x);
    else
    {
        /*
        当我们
        add r0, r1, c
        时，假如c过大，那么如下处理
        op: add r0, r1, c & 0xff
        o2: add r0, r0, c & 0xff00
        o2: add r0, r0, c & 0xff0000
        o2: add r0, r0, c & 0xff000000
        */
        uint32_t a[24], o2, no; 
        int i;
        a[0] = 0xff << 24;
        /* o2的源操作和目的操作数需要修正成同一个 */
        o2 = (op & 0xfff0ffff) | (op & 0x0f00);
        no = o2 ^ 0xa00000;
        for (i = 1; i < 24; i++)
            a[i] = a[i - 1] >> 1;

        o(stuff_const(op, v & 0xff));
        if (!(v & 0x8000) || !(v & 0x800000) || !(v & 0x80000000)) vm_error("stuff_const cant fit instruction");
        o(stuff_const(o2, v & 0xff00));
        if (!(v & 0x8000)) 
            o(stuff_const(no, v & 0x8000));
        o(stuff_const(o2, v & 0xff0000));
        if (!(v & 0x800000))
            o(stuff_const(no, v & 0x800000));
        o(stuff_const(o2, v & 0xff000000));
        if (!(v & 0x80000000))
            o(stuff_const(no, v & 0x80000000));
    }
}

int _push(int32_t reglist)
{
#define T1_REGLIST_MASK             0x40ff         
#define T2_REGLIST_MASK             0x5fff

    if (ANDNEQ(reglist, T1_REGLIST_MASK)) 
        o(0xb400 | (reglist & 0xff) | ((reglist & 0x4000) ? 0x100:0));
    else if (ANDNEQ(reglist, T2_REGLIST_MASK)) 
        o(0xe92d0000 | reglist);
    return 0;
}

void _pop(int32_t reglist)
{
    /* A8.8.131 */
    if (ANDNEQ(reglist, 0x80ff)) {
        if (bitcount(reglist) < 1) UNPREDICITABLE();
        o(0xbc00 | (reglist & 0xff) | ((reglist & 0x8000) ? 0x100 : 0));
    }
    else if (ANDNEQ(reglist, 0xdfff)) {
        if (bitcount(reglist) < 2 || ANDEQ(reglist, 0xc000)) UNPREDICITABLE();
        o(0xe8bd0000 | (reglist & 0xdfff));
    }
}

int _add(int rd, int rn, uint32_t imm)
{
    if (rn == SP) {
        if (align4(imm) && (imm < 1024) && (rd < 8))
            o(0xa800 | (rd << 8) | (imm >> 2));
        else if ((rd == SP) && align4(imm) && (imm < 512))
            o(0xb000 | (imm >> 2));
        else if (imm < 4096)
            o(stuff_constw(0xf20d0000 | (rd << 8), imm, 12));
        else
            stuff_const_harder(0xf10d0000, imm);
    }

    return 0;
}

int _ldr(int rt, int rn, int rm, int imm, char *buf)
{
    if (rm != -1) {
    }
    else if (in_imm3(rt) && in_imm3(rn)){
        if (!align4(imm) || !in_imm5(imm >> 2)) UNPREDICITABLE();
        o(0x68 | ((imm >> 2) << 6) | (rn << 3) | rt);
    }

    return 0;
}

int _str(int rt, int rn, int rm, int imm)
{
    if (rm != -1) {
    }
    else if (rt < 8 && rn < 8) {
        if (align4(imm) && imm < 128)
            o(0x6000 | ((imm >> 2) << 6) | (rn << 3) | rt);
    }
    else if ((rn == SP) && rt < 8 && align4(imm) && imm < 1024)
        o(0x9000 | (rt << 8) | (imm >> 2));
    else if (imm < 4096)
        o(0xf8600000 | (rn << 16) | (rt << 12) | imm);

    return 0;
}

void _mov(int rd, uint32_t v)
{
    /* A8.8.102 */
    if (in_imm8(v) && in_imm3(rd)) 
        o(0x2000 | (rd << 8) | v); // T1
    else {
        uint32_t x = stuff_const(0xf04f0000 | (rd << 8), v); // T2
        if (x) {
            o(x);
            return;
        }

        /* 
        碰见超大数
        mov rd, v.lo
        movt rd, v.hi
        */
        o(stuff_constw(0xf2400000 | (rd << 8), v & 0xffff, 16));
        v >>= 16;
        o(stuff_constw(0xf2c00000 | (rd << 8), v, 16));
    }
}

uint32_t encbranch(int pos, int addr, int fail)
{
    /* A8.8.18 */
    addr -= pos + 4;
    addr /= 2;
    if ((addr >= 0xEFFF) || addr < -0xEFFF) {
        vm_error("FIXME: function bigger than 1MB");
        return 0;
    }
    return 0xf0008000 | imm_map(addr, 31, 1, 26) | imm_map(addr, 18, 1, 11) | imm_map(addr, 17, 1, 13) | imm_map(addr, 11, 6, 16) | imm_map(addr, 0, 11, 0);
}

uint32_t encbranch2(int pos, int addr, int arm)
{
    addr -= pos + 4;
    addr /= 2;
    if ((addr >= 0xffffff) && (addr < -0xffffff))
        vm_error("FIXME: jmp bigger than 16MB");

    int s = bit_get(addr, 31, 1), j1, j2, i1 = bit_get(addr, 22, 1), i2 = bit_get(addr, 21, 1);

    j1 = (!i1) ^ s;
    j2 = (!i2) ^ s;

    return  (s << 26) | (j1 << 13) | (j2 << 11) | imm_map(addr, 11, 10, 16) | imm_map(addr, 0, 11, 0);
}

pit thumb_gen::g_push(flowblock *b, pit pit)
{
    int reglist = 0;

    pcodeop *p = *pit, *p1;

    if ((p->opcode == CPUI_COPY) && p->get_in(0)->get_addr() == d->sp_addr) {
        p = *++pit;
    }

    while (p->output->get_addr() != d->sp_addr) {
        p = *pit++;
        p1 = *pit++;
        if ((p->opcode == CPUI_INT_SUB) && (p1->opcode == CPUI_STORE) && (pi1a(p1) == ama)) {
            reglist |= 1 << reg2index(pi2a(p1));
        }
        else throw LowlevelError("not support");

        p = *pit;
    }

    if ((poa(p) == asp) && (pi0a(p) == ama)) {
        if ((p->opcode == CPUI_COPY)
            /* FIXME: rn = rm + 0 == > rn = copy rm*/
            || ((p->opcode == CPUI_INT_ADD) && pi1(p)->is_constant() && !pi1(p)->get_val()))
            _push(reglist);
    }

    return pit;
}

pit thumb_gen::g_pop(flowblock *b, pit pit)
{
    int reglist = 0;
    pcodeop *p = *pit, *p1;

    while (poa(p) == asp || poa(p) == ama) {
        p = *pit++;
        p1 = *pit++;
        if ((p->opcode == CPUI_INT_ADD)) {
            if (p1->opcode != CPUI_LOAD)
                break;
            reglist |= 1 << reg2index(poa(p1));
        }
        else throw LowlevelError("not support");

        p = *pit;
    }

    if ((p1->opcode == CPUI_COPY)) {
        _pop(reglist);

        if ((reglist & (1 << PC))) {
            if ((*pit++)->opcode != CPUI_INT_AND
                || (*pit)->opcode != CPUI_RETURN)
                vm_error("pop need pc operation ");
        }

        return pit;
    }
    vm_error("pop meet error pcode, %d", p->start.getTime());
    return pit;
}

int thumb_gen::run()
{
    int i, allsiz = 0;
    uint32_t x;
    flowblock *b, *b1;

    /* 1. 设置写入位置
    2. 清空原函数 */
    data = d->loader->filedata;
    ind = fd->bufptr - data;

    ind -= fd->flags.thumb;

    memset(data, 0, fd->size);
    /* 对block进行排序 */
    /* FIXME:这里直接用最土炮的方法来做了，实际上应该用topsort来做会好点，更具体的请参考:

    Linear Scan Register Allocation for the Java HotSpot™ Client Compiler
    */
    blist = fd->bblocks.blist;

    /* 针对每一个Block生成代码，但是不处理末尾的jmp */
    for (i = 0; i < blist.size(); i++) {
        b = blist[i];

        run_block(b);

        if (b->out.size() == 0) continue;

        if (b->out.size() == 1) {
            if (((i + 1) == blist.size()) || (blist[i + 1] != b->get_out(0))) {
                b1 = b->get_out(0);
                if (b1->cg.data) {
                    x = encbranch(ind, b->cg.data - data, 0);
                    o(x | (COND_AL << 22));
                }
                else {
                    o(NOP2);
                }
            }
        }
        else if (b->out.size() == 2) {
        }
        else 
            throw LowlevelError("now not support switch code gen");
    }

    /* 修复末尾的跳转 */
    for (i = 0; i < flist.size(); i++) {
        fix_item *item = flist[i];

        /* 读取出原先的inst，然后取出其中的cond，生成新的inst，然后写入buf */
        x = read_thumb2(data + item->ind);
        x =  (x & 0x03c00000)| encbranch(item->ind, item->b->cg.data - data, 0);
        write_thumb2(x, data + item->ind);
    }

    return 0;
}

int thumb_gen::run_block(flowblock *b)
{
    list<pcodeop *>::iterator it, it1;
    pcodeop *p, *p1, *p2;
    uint32_t reg, x, rt, rd, rn, rm, setflags;
    int oind, imm, target_thumb ;

    b->cg.data = data + ind;

    for (it = b->ops.begin(); it != b->ops.end(); ++it) {
        p = *it;
        oind = ind;

        /* branch 指令不处理，由外层循环进行填充 */
        if (p->opcode == CPUI_BRANCH) continue;
        /* phi节点不处理 */
        if (p->opcode == CPUI_MULTIEQUAL) continue;

        switch (p->opcode) {
        case CPUI_COPY:
            /* push */
            if (poa(p) == ama) it = g_push(b, it);
            else if (poa(p) == alr) {
                p1 = *++it;
                if ((p1->opcode == CPUI_CALL) && (pi0(p1)->get_addr().getSpace() == d->ram_spc)) {
                    imm = pi0(p1)->get_addr().getOffset();
                    target_thumb = d->func_is_thumb(pi0(p1)->get_val());
                    /* A8.8.25 */
                    if (fd->flags.thumb ^ target_thumb)
                        o(0xf000c000 | encbranch2(ind, imm, !target_thumb));
                    else 
                        o(0xf000d000 | encbranch2(ind, imm, !target_thumb));
                }
            }
            else if (pi0(p)->is_constant())
                _mov(reg2index(poa(p)), pi0(p)->get_val());
            else if (isreg(p->output) && isreg(pi0(p))) {
                rd = reg2index(poa(p));
                /* A8.8.103*/
                o(0x4600 | (reg2index(pi0a(p)) << 3) | (rd & 7) | (rd >> 3 << 7));
            }
            break;

        case CPUI_LOAD:
            if (isreg(p->output) && pi1(p)->is_constant()) {
                rd = reg2index(poa(p));
                _mov(rd, pi1(p)->get_val());
                o(0xf8d00000 | (rd << 12) | (rd << 16) | 0);
            }
            break;

        case CPUI_INT_NOTEQUAL:
        case CPUI_INT_EQUAL:
            if (istemp(p->output) && (pi0a(p) == azr) && pi1(p)->is_constant() && (pi1(p)->get_val() == 0)) {
                p1 = *++it;
                if (p1->opcode == CPUI_BOOL_NEGATE) {
                    p2 = *++it;
                    if (p2->opcode == CPUI_CBRANCH) {
                        flowblock *t = b->get_true_edge()->point;
                        x = (((p->opcode == CPUI_INT_NOTEQUAL)?COND_EQ:COND_NE) << 22);
                        x |=  t->cg.data ?  encbranch(ind, t->cg.data - data, 0):encbranch(0, 0, 0);
                        if (!t->cg.data)
                            add_fix_list(ind, t);
                        o(x);
                    }
                }
            }
            break;

        case CPUI_INT_SUB:
            if (poa(p) == ama) it = g_push(b, it);
            else if (istemp(p->output)) {
                p1 = *++it;
                if (p1->opcode == CPUI_INT_EQUAL) {
                    p2 = *++it;
                    if ((p2->opcode == CPUI_COPY) && (poa(p2) == azr)) {
                        reg = reg2index(pi0a(p));
                        if (pi1(p)->is_constant() && (pi1(p)->get_val() < 256) && (reg < 7))
                            o(0x2800 | (reg << 8) | ((uint32_t)pi1(p)->get_val()));
                    }
                }
            }
            else if (isreg(p->output)) {
                if (isreg(pi0(p))) {
                    if (isreg(pi1(p))) {
                        /* A8.8.223 */
                        rn = reg2index(pi0a(p));
                        rm = reg2index(pi1a(p));
                        rd = reg2index(poa(p));

                        it1 = it;
                        p1 = *++it1;
                        setflags = 0;
                        if ((p1->opcode == CPUI_INT_EQUAL) && (p->output == pi0(p1))) {
                            p2 = *++it1;
                            it = it1;
                            setflags = 1;
                        }

                        if (setflags && rn < 8 && rm < 8 && rd < 8)
                            o(0x1a00 | rd | (rn << 3) || (rm << 6));
                        else
                            o(0xeba00000 | rm | (rd << 8) | (rn << 16) || (setflags << 20));
                    }
                    /* sub sp, sp, c 
                    A8.8.225.T1
                    */
                    else if (pi1(p)->is_constant() && (pi0a(p) == asp) && (poa(p) == asp)) {
                        imm = pi1(p)->get_val();
                        if (!(imm & 3)) /* ALIGN 4 */
                            o(0xb080 | (imm >> 2));
                        else if (imm < 4096) //subw, T3
                            o(stuff_constw(0xf25b0000, imm, 12));
                        else // T2
                            o(stuff_const(0xf1ad0000 | (SP << 8), imm));
                    }
                }
            }
            break;


        case CPUI_INT_ADD:
            if (istemp(p->output)) {
                p1 = *++it;
                switch (p1->opcode) {
                case CPUI_COPY:
                    if ((pi0a(p) == asp) && pi1(p)->is_constant() && a(pi1(p1)) == poa(p))
                        _add(reg2index(poa(p1)), SP, pi1(p)->get_val());
                    break;

                case CPUI_STORE:
                    if (a(pi1(p1)) == poa(p))
                        _str(reg2index(pi2a(p1)), reg2index(pi0a(p)), -1, pi1(p)->get_val());
                    break;

                case CPUI_LOAD:
                    if (a(pi1(p1)) == poa(p)) {
                        rt = reg2index(poa(p1));
                        /* A8.8.62 */
                        if ((pi0a(p) == asp) && (pi1(p)->get_val() < 256) && (rt < 8))
                            o(0x9800 | (rt << 8) | pi1(p)->get_val());
                        else
                            o(0xf8d00000 | (rt << 12) | (reg2index(pi0a(p)) << 16) | pi1(p)->get_val());
                    }
                    break;
                }
            }
            else if ((pi0a(p) == asp) && pi1(p)->is_constant()) {
                it1 = it;
                p1 = *++it1;
                if (p1->opcode == CPUI_LOAD)
                    it = g_pop(b, it);
                else
                    _add(reg2index(poa(p)), SP, pi1(p)->get_val());
            }
            else if (isreg(p->output)) {
                rd = reg2index(poa(p));
                rn = reg2index(pi0a(p));
                /* A8.8.4 */
                if (pi2(p)->is_constant()) {
                    imm = (int)pi1(p)->get_val();
                    if ((poa(p) == pi0a(p)) && rd < 8 && imm < 256)
                        o(0x3000 | (rd << 8) | (imm & 0xff));       // T2
                    else {
                        x = stuff_const(0xf1000000, imm);           // T3
                        if (!x) {
                            //x = 0xf2000000 | imm_map(imm, );
                        }

                        x |= (rn << 16) | (rd << 8);
                        o(x);
                    }
                }
            }
            break;

        case CPUI_INT_AND:
            if (poa(p) == apc) {
                p1 = *++it;
                if (poa(p1) == alr) {
                    p2 = *++it;
                    if (p2->opcode == CPUI_CALLIND) 
                        o(0x4780 | (reg2index(pi0a(p)) << 3));
                }
            }
            else if (istemp(p->output)) {
                p1 = *++it;
                if (p1->opcode == CPUI_STORE) {
                    if (isreg(pi0(p)) && pi1(p)->is_constant()) {
                        imm = pi1(p)->get_val();
                        rt = reg2index(pi2a(p1));
                        rn = reg2index(pi0a(p));
                        _str(rt, rn, -1, imm);
                    }
                }
            }
            break;

        default:
            break;
        }

        if (oind == ind)
            vm_error("thumbgen find un-support pcode seq[%d]", p->start.getTime());

        /* 打印新生成的代码 */
#if 1
        int len = ind - oind, i, siz;

        while (len > 0) {
            AssemblyRaw assem;
            assem.disbable_html();
            char buf[128];
            assem.set_buf(buf);
            siz = d->trans->printAssembly(assem, Address(d->trans->getDefaultCodeSpace(), oind));

            printf("[p%4d] ", p->start.getTime());

            for (i = 0; i < siz; i++)
                printf("%02x ", data[oind + i]);

            for (; i < 4; i++)
                printf("   ");

            puts(buf);

            len -= siz;
            oind += siz;
        }
#endif
    }
    return 0;
}

void thumb_gen::add_fix_list(int ind, flowblock *b)
{
    flist.push_back(new fix_item(ind, b));
}

void thumb_gen::dump()
{
    funcdata *fd1 = new funcdata(fd->name.c_str(), fd->get_addr(), 0, d);

    fd1->flags.dump_inst = 1;
    fd1->follow_flow();
    fd1->heritage();
    fd1->dump_cfg(fd1->name, "after_codegen", 1);
}
