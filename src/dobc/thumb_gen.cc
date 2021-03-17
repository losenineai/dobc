
#include "sleigh.hh"
#include "thumb_gen.hh"
#include <assert.h>

#define pi0(p)              p->get_in(0)
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
#define istemp(vn)          ((vn)->get_addr().getSpace()->getType() == IPTR_INTERNAL)

#define ANDNEQ(r1, r2)      ((r1 & ~r2) == 0)
#define in_imm8(a)          ANDNEQ(a, 0xff)
#define in_imm7(a)          ANDNEQ(a, 0x7f)

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


int thumb_gen::reg2index(const Address &a)
{
    return (a.getOffset() - ar0.getOffset()) / 4;
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
            t16 = 0xa800 | (rd << 7) | imm;
        }
    }

    thumb_end_copy();
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
    pcodeop *p = *pit, *p1;

    if (istemp(p->output)) {
        p1 = *++pit;
        if ((pi0a(p) == asp) && pi1(p)->is_constant() && (p1->opcode == CPUI_COPY) && a(pi1(p1)) == poa(p)) {
            asmlen += _add(reg2index(poa(p)), 1, pi1(p)->get_val(), asmbuf);
        }
        else
            UNPREDICITABLE();
    }

    return pit;
}


int thumb_gen::run()
{
    int i;
    list<pcodeop *>::iterator it, it1;
    pcodeop *p;
    for (i = 0; i < blist.size(); i++) {
        flowblock *b = blist[i];

        for (it = b->ops.begin(); it != b->ops.end(); ++it) {
            p = *it;
            it1 = it;
            switch (p->opcode) {
            case CPUI_COPY:
                if (poa(p) == ama) it = g_push(b, it);
                break;

            case CPUI_INT_SUB:
                if (poa(p) == ama) it = g_push(b, it);
                break;

            case CPUI_INT_ADD:
                it = g_add(b, it);
                break;

            default:
                break;
            }

            if (it1 == it) {
                throw LowlevelError("pcodeop iterator not increment");
            }
        }
    }
    return 0;
}
