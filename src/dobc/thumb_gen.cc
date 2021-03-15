
#include "sleigh.hh"
#include "thumb_gen.hh"

#define pvn0(p)             p->get_in(0)
#define pvn1(p)             p->get_in(1)
#define pvn2(p)             p->get_in(2)
#define pvn0a(p)            p->get_in(0)->get_addr()
#define pvn1a(p)            p->get_in(1)->get_addr()
#define pvn2a(p)            p->get_in(2)->get_addr()
#define ama                 d->ma_addr
#define ar0                 d->r0_addr

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

int thumb_gen::run()
{
    int i;
    list<pcodeop *>::iterator it;
    pcodeop *p;
    for (i = 0; i < blist.size(); i++) {
        flowblock *b = blist[i];

        for (it = b->ops.begin(); it != b->ops.end(); ++it) {
            p = *it;
            switch (p->opcode) {
            case CPUI_COPY:
                if (p->output->get_addr() == d->ma_addr) {
                    it = g_push(b, it);
                }
                break;

            default:
                throw LowlevelError("not support");
            }
        }
    }
    return 0;
}

int thumb_gen::reg2index(const Address &a)
{
    return (a.getOffset() - ar0.getOffset()) / 4;
}

int thumb_gen::_push(int32_t reglist, char *buf, int size)
{
#define T1_REGLIST_MASK             0x40ff         
#define T2_REGLIST_MASK             0x5fff

    if (reglist & T1_REGLIST_MASK) {
    }
    return 0;
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
        if ((p->opcode == CPUI_INT_SUB) && (p1->opcode == CPUI_STORE) && (pvn1a(p1) == ama)) {
            reglist |= reg2index(pvn2a(p1));
        }
        else throw LowlevelError("not support");
    }

    return pit;
}
