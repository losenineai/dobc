
#include "vm.h"
#include "pass.hh"
#include "thumb_gen.hh"

int pass_cond_reduce::run()
{
    return 0;
}

int pass_regalloc_const_arm::run()
{
    list<pcodeop *>::iterator it;
    vector<pcodeop *> subs;
    int i, rn, rm, changed = 0;
    pcodeop *p, *p1;
    bitset<32> live_out;
    pcodefunc pf(fd);

    fd->bblocks.compute_local_live_sets();
    fd->bblocks.compute_global_live_sets();
    fd->bblocks.dump_live_sets();
    fd->bblocks.collect_cond_copy_sub(subs);

    if (subs.empty()) return 0;

    /* 
    把
    cmp rn, XXXXX
    cbranch
    形式转换成
    mov rm, xxxx
    cmp rn, rm
    cbranch
    */
    for (i = 0; i < subs.size(); i++) {
        p = subs[i];
        /* A8.8.37 填充的const 在 2048以内*/
        if (thumb_gen::stuff_const(0, pi1(p)->get_val())) continue;

        live_out = p->parent->live_out;
        rn = d->reg2i(pi0a(p));
        live_out.set(rn);
        live_out.flip();

        rm = ntz((uint32_t)live_out.to_ulong());
        if (rm >= SP)
            vm_error("pass_regalloc_const_arm() failure");

        it = p->basiciter;
        pf.add_copy_const(p->parent, it, varnode(fd, 4, d->i2reg(rm)), *pi1(p));

        p1 = *--it;
        fd->op_remove_input(p, 1);
        fd->op_set_input(p, p1->output, 1);
        changed = 1;
    }

    if (changed)
        fd->structure_reset();

    return changed;
}
