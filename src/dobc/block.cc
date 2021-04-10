
#include "sleigh.hh"
#include "dobc.hh"
#include "thumb_gen.hh"

void        blockgraph::compute_local_live_sets(void)
{
    int i, j, r;
    list<pcodeop *>::iterator it;
    flowblock *b;

    vector<int> scratch_regs;

    d->get_scratch_regs(scratch_regs);

    for (i = 0; i < blist.size(); i++) {
        b = blist[i];

        b->live_gen.reset();
        b->live_kill.reset();
        for (it = b->ops.begin(); it != b->ops.end(); it++) {
            pcodeop *p = *it;
            for (j = 0; j < p->inrefs.size(); j++) {
                r = d->reg2i(p->get_in(j)->get_addr());
                if (r == -1) continue;

                if (!b->live_kill.test(r)) b->live_gen.set(r);
            }

            if (p->is_call()) {
                for (j = 0; j < scratch_regs.size(); j++)
                    b->live_kill.set(scratch_regs[j]);
            }

            if (p->output && ((r = d->reg2i(p->output->get_addr())) >= 0)) 
                b->live_kill.set(r);
        }
    }
}

void        blockgraph::compute_global_live_sets(void)
{
    int i, j, changed = 0;
    flowblock *b, *outb;
    bitset<32> live_out0, live_in0;

    do {
        changed = 0;
        for (i = blist.size() - 1; i >= 0; i--) {
            b = blist[i];
            live_out0 = b->live_out;
            b->live_out.reset();
            for (j = 0; j < b->out.size(); j++) {
                outb = b->get_out(j);
                b->live_out |= outb->live_in;
            }

            live_in0 = b->live_in;
            b->live_in = (b->live_out & ~b->live_kill) | b->live_gen;

            if (!changed && ((live_out0 != b->live_out) || (live_in0 != b->live_in)))
                changed = 1;
        }
    } while (changed);
}

void        blockgraph::dump_live_sets()
{
    int i;

    printf("====================dump_live_set_begin============\n");
    for (i = 0; i < blist.size(); i++)
        dump_live_set(blist[i]);
    printf("====================dump_live_set_end============\n");
}

void        blockgraph::dump_live_set(flowblock *b)
{
    printf("block{id:%d, dfnum:%d} liveout[%lx] \n", b->index, b->dfnum, b->live_out.to_ulong());
}
