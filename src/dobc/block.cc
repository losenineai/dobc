
#include "sleigh.hh"
#include "dobc.hh"
#include "thumb_gen.hh"

void        blockgraph::collect_cond_copy_sub(vector<pcodeop *> &subs)
{
    int i;
    flowblock *b;
    list<pcodeop *>::reverse_iterator   it;
    pcodeop *p;

    for (i = 0; i < blist.size(); i++) {
        b = blist[i];
        if (b->out.size() != 2) continue;
        if (b->ops.size() <= 4) continue;

        it = b->ops.rbegin();
        advance(it, 3);
        p = *it;
        if ((p->opcode == CPUI_INT_SUB) && d->is_temp(poa(p)) && pi1(p)->is_constant())
            subs.push_back(p);
    }
}

void        blockgraph::collect_no_cmp_cbranch_block(vector<flowblock *> &blks)
{
    for (int i = 0; i < blist.size(); i++) {
        flowblock *b = blist[i];
        if (!b->is_cbranch()) continue;
        if (b->in.size() < 3) continue;

        pcodeop *sub = b->get_cbranch_sub_from_cmp();
        if (sub) continue;

        blks.push_back(b);
    }
}

flowblock*  blockgraph::get_it_end_block(flowblock *b)
{
    return (b->get_out(0)->get_out(0) == b->get_out(1)) ? b->get_out(1):b->get_out(0);
}

void        blockgraph::collect_sideeffect_ops()
{
    list<pcodeop *>::iterator it;

    for (int i = 0; i < blist.size(); i++) {
        flowblock *b = blist[i];

        b->sideeffect_ops.clear();
        for (it = b->ops.begin(); it != b->ops.end(); it++) {
            pcodeop *p = *it;

            if ((p->opcode == CPUI_STORE) || (p->opcode == CPUI_CALL) || (p->opcode == CPUI_CALLIND)) {
                p->sideiter = b->sideeffect_ops.insert(b->sideeffect_ops.end(), p);
            }
        }
    }
}

flowblock*  blockgraph::find_post_dom(flowblock *f)
{
    return NULL;
}

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

            /* fix interval */
            if (b->is_end() && !b->noreturn()) 
                b->live_out |= 0xffff;

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
    //printf("block{id:%d, dfnum:%d} liveout[%lx] \n", b->index, b->dfnum, b->live_out.to_ulong());
    printf("block{id:%3d, dfnum:%3d} liveout[%s] \n", b->index, b->dfnum, b->live_out.to_string().c_str());
}

void        blockgraph::dump_live_set(pcodeop *p)
{
    printf("pcode{uniq:p%3d} livein[%s] liveout[%s] \n", p->start.getTime(), p->live_in.to_string().c_str(), p->live_out.to_string().c_str());
}

void        blockgraph::compute_local_live_sets_p(void)
{
    int i, j, r;
    list<pcodeop *>::reverse_iterator it;
    flowblock *b;

    vector<int> scratch_regs;

    d->get_scratch_regs(scratch_regs);

    for (i = 0; i < blist.size(); i++) {
        b = blist[i];

        for (it = b->ops.rbegin(); it != b->ops.rend(); it++) {
            pcodeop *p = *it;

            for (j = 0; j < p->inrefs.size(); j++) {
                r = d->reg2i(p->get_in(j)->get_addr());
                if (r == -1) continue;

                if (!p->live_kill.test(r)) p->live_gen.set(r);
            }

            if (p->is_call()) {
                for (j = 0; j < scratch_regs.size(); j++)
                    p->live_kill.set(scratch_regs[j]);
            }

            if (p->output && ((r = d->reg2i(p->output->get_addr())) >= 0)) 
                p->live_kill.set(r);
        }
    }
}

void        blockgraph::compute_global_live_sets_p(void)
{
    int i, j, changed = 0;
    flowblock *b;
    bitset<32> live_out0, live_in0;
    list<pcodeop *>::reverse_iterator   it;
    pcodeop *p, *prev;

    do {
        changed = 0;
        for (i = blist.size() - 1; i >= 0; i--) {
            b = blist[i];

            for (it = b->ops.rbegin(); it != b->ops.rend(); it++) {
                p = *it;

                if (!changed) {
                    live_out0 = p->live_out;
                    live_in0 = p->live_in;
                }

                if ((it == b->ops.rbegin())) {
                    /* fix interval */
                    if (b->is_end()) {
                        if (!b->noreturn())
                            p->live_out |= 0xffff;
                    }
                    else {
                        for (j = 0; j < b->out.size(); j++) 
                            p->live_out |= b->get_out(j)->ops.front()->live_in;
                    }
                }
                else {
                    p->live_out = prev->live_in;
                }

                p->live_in = (p->live_out & ~p->live_kill) | p->live_gen;

                if (!changed && ((live_out0 != p->live_out) || (live_in0 != p->live_in)))
                    changed = 1;

                prev = p;
            }
        }
    } while (changed);
}
