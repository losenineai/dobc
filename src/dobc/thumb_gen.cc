
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

#define read_thumb2(b)          ((((uint32_t)(b)[0]) << 16) | (((uint32_t)(b)[1]) << 24) | ((uint32_t)(b)[2]) | (((uint32_t)(b)[3]) << 8))
#define write_thumb2(b)

thumb_gen::thumb_gen(funcdata *f)
{
    fd = f;
    d = fd->d;
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

#define thumb_end_copy()        do { \
        if (t16) { \
            *(uint16_t *)buf = t16; \
            return 2; \
        } \
        else if (t32) { \
            *(uint32_t *)buf = t32; \
            return 4; \
        } \
 \
        UNPREDICITABLE(); \
        return -1; \
    } while (0)

#define thumb_pre()             \
        uint32_t t32= 0;     \
        uint16_t t16 = 0;

static  thumb_gen *cur_func = NULL;

static void ot(uint16_t i)
{
    cur_func->data[cur_func->ind++] = i & 255;
    i >>= 8;
    cur_func->data[cur_func->ind++] = i & 255;
}

static void ott(uint32_t i)
{
    cur_func->data[cur_func->ind + 2] = i & 255;
    i >>= 8;
    cur_func->data[cur_func->ind + 3] = i & 255;
    i >>= 8;
    cur_func->data[cur_func->ind + 1] = i & 255;
    i >>= 8;
    cur_func->data[cur_func->ind + 0] = i & 255;
    i >>= 8;

    cur_func->ind += 4;
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

#define imm_map(imm, l, bw, r)          (((imm >> l) & (2 ^ bw - 1)) << r)

static void stuff_const_harder(uint32_t op, uint32_t v)
{
    uint32_t x, h, o1;
    x = stuff_const(op, v);
    if (x)
        o(x);
    else if (o1 = stuff_const(op, v & 0xffff)) {
        o(o1);
        h = v >> 16;
        /* A8.8.106, movt*/
        o((op & 0x0f00) | 0xf2c00000 | (h & 0xff) | imm_map(h, 0, 8, 0) | imm_map(h, 8, 3, 12) | imm_map(h, 11, 1, 26) | imm_map(h, 12, 4, 16));
    }
    else
    {
        UNPREDICITABLE();
        //uint32_t a[16], nv, no, o2, n2;
    }
}

int _push(int32_t reglist, char *buf)
{
#define T1_REGLIST_MASK             0x40ff         
#define T2_REGLIST_MASK             0x5fff

    uint16_t t16 = 0;
    uint32_t t32 = 0;

    if (ANDNEQ(reglist, T1_REGLIST_MASK)) {
        if (reglist & 0x4000) t16 |= 0x100;
        t16 |= 0xb400 | (reglist & 0xff);
    }
    else if (ANDNEQ(reglist, T2_REGLIST_MASK)) {
        if (reglist & (1 << 9))    t16 |= 1 << 13;
        t32 |= 0xe92d | (reglist & 0xfff);
    }
    else {
        assert(0);
    }

    thumb_end_copy();
}

int _add(int rd, int sp, int imm, char *buf)
{
    uint16_t t16 = 0;
    uint32_t t32 = 0;

    if (sp) {
        if (in_imm8(imm) && (rd <= 7)) {
            t16 = 0xa800 | (rd << 8) | imm;
        }
    }

    thumb_end_copy();
}

int _ldr(int rt, int rn, int rm, int imm, char *buf)
{
    uint16_t t16 = 0;
    uint32_t t32 = 0;

    if (rm != -1) {
    }
    else if (in_imm3(rt) && in_imm3(rn)){
        if (!align4(imm) || !in_imm5(imm >> 2)) UNPREDICITABLE();

        t16 = 0x68 | ((imm >> 2) << 6) | (rn << 3) | rt;
    }

    thumb_end_copy();
}

int _str(int rt, int rn, int rm, int imm, char *buf)
{
    uint16_t t16 = 0;
    uint32_t t32 = 0;

    if (rm != -1) {
    }
    else if (in_imm3(rt) && in_imm3(rn)){
        if (!align4(imm) || !in_imm5(imm >> 2)) UNPREDICITABLE();

        t16 = 0x60 | ((imm >> 2) << 6) | (rn << 3) | rt;
    }

    thumb_end_copy();
}

int _mov(int rd, uint32_t imm)
{
    if (in_imm8(imm) && in_imm3(rd)) 
        o(0x2000 | (rd << 8) | imm);
    else 
        stuff_const_harder(0xf04f0000 | rd << 8, imm);

    return 0;
}

uint32_t encbranch(int pos, int addr, int fail)
{
    addr -= pos + 4;
    addr /= 2;
    if ((addr >= 0xEFFF) || addr < -0xEFFF) {
        vm_error("FIXME: function bigger than 1MB");
        return 0;
    }
    return 0xf00080000 | imm_map(addr, 31, 1, 26) | imm_map(addr, 18, 1, 11) | imm_map(addr, 17, 1, 13) | imm_map(addr, 11, 6, 16) | imm_map(addr, 0, 11, 0);
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
            reglist |= reg2index(pi2a(p1));
        }
        else throw LowlevelError("not support");
    }

    asmlen += _push(reglist, asmbuf);

    return pit;
}

pit thumb_gen::g_add(flowblock *b, pit pit)
{
    pcodeop *p = *pit++, *p1;

    p1 = p;

    if (istemp(p->output)) {
        p1 = *pit;

        if ((pi0a(p) == asp) && pi1(p)->is_constant() && (p1->opcode == CPUI_COPY) && a(pi1(p1)) == poa(p)) {
            asmlen += _add(reg2index(poa(p1)), 1, pi1(p)->get_val(), asmbuf);
        }
        else
            UNPREDICITABLE();
    }
    else if ((pi0a(p) == asp) && pi1(p)->is_constant()){
        asmlen += _add(reg2index(poa(p)), 1, pi1(p)->get_val(), asmbuf);
    }

    return pit;
}

pit thumb_gen::g_sub(flowblock *b, pit pit)
{
    return pit;
}

pit thumb_gen::g_ldr(flowblock *b, pit pit)
{
    pcodeop *p = *pit++, *p1;

    p1 = *pit;

    return pit;
}

pit thumb_gen::g_str(flowblock *b, pit pit)
{
    return pit;
}

pit thumb_gen::g_mov(flowblock *b, pit pit)
{
    return pit;
}

pit thumb_gen::g_blx(flowblock *b, pit pit)
{
    return pit;
}

int thumb_gen::run()
{
    int i, allsiz = 0;
    uint32_t x;
    flowblock *b, *b1;
    /* 对block进行排序 */

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
            flowblock *f = b->get_false_edge()->point;

            if (((i + 1) == blist.size()) && (blist[i + 1] != b->get_out(0))) 
                b->flags.g_flow_branch = 1;
        }
        else {
            throw LowlevelError("now not support switch code gen");
        }
    }

    /* 修复末尾的跳转 */
    for (i = 0; i < flist.size(); i++) {
        fix_item *item = flist[i];

        x = read_thumb2(data + item->ind);
        x |= encbranch(item->ind, item->b->cg.data - data, 0);
    }

    return 0;
}

int thumb_gen::run_block(flowblock *b)
{
    list<pcodeop *>::iterator it, it1;
    pcodeop *p, *p1, *p2;
    uint32_t reg, x;
    int plen;

    b->cg.data = data + ind;

    for (it = b->ops.begin(); it != b->ops.end(); ++it) {
        p = *it;
        plen = asmlen;
        switch (p->opcode) {
        case CPUI_COPY:
            if (poa(p) == ama) it = g_push(b, it);
            else if (pi1(p)->is_constant())
                _mov(reg2index(poa(p)), pi1(p)->get_val());
            break;

        case CPUI_INT_EQUAL:
            if (istemp(p->output) && (pi0a(p) == azr) && pi1(p)->is_constant() && (pi1(p)->get_val() == 0)) {
                p1 = *++it;
                if (p1->opcode == CPUI_BOOL_NEGATE) {
                    p2 = *++it;
                    if (p2->opcode == CPUI_CBRANCH) {
                        flowblock *t = b->get_true_edge()->point;
                        x = (COND_EQ << 22);
                        if (t->cg.data) 
                            x |= encbranch(ind, t->cg.data - data, 0);
                        else 
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
            break;

        case CPUI_INT_ADD:
            if (istemp(p->output)) {
                p1 = *++it;
                switch (p1->opcode) {
                case CPUI_COPY:
                    if ((pi0a(p) == asp) && pi1(p)->is_constant() && a(pi1(p1)) == poa(p)) 
                        asmlen += _add(reg2index(poa(p1)), 1, pi1(p)->get_val(), asmbuf);
                    break;

                case CPUI_STORE:
                    if (a(pi1(p1)) == poa(p)) {
                        asmlen += _str(reg2index(pi2a(p1)), reg2index(pi0a(p)), -1, pi1(p)->get_val(), asmbuf);
                    }
                    break;
                }
            }
            else if ((pi0a(p) == asp) && pi1(p)->is_constant()){
                asmlen += _add(reg2index(poa(p)), 1, pi1(p)->get_val(), asmbuf);
            }
            break;

        case CPUI_INT_AND:
            if (poa(p) == apc) {
                p1 = *++it;
                if (poa(p1) == alr) {
                    p2 = *++it;
                    if (p2->opcode == CPUI_CALL) 
                        o(0x4780 | (reg2index(pi0a(p)) << 3));
                }
            }
            break;

        default:
            break;
        }

        if (plen == asmlen) {
            throw LowlevelError("pcodeop iterator not increment");
        }
    }
    return 0;
}

void thumb_gen::add_fix_list(int ind, flowblock *b)
{
    flist.push_back(new fix_item(ind, b));
}
