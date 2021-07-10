
#include "codegen.hh"

cgtrie_node::cgtrie_node()
{
    nodes.resize(CPUI_MAX);
}

void codegen::sort_blocks(vector<flowblock *> &blks)
{
    int i, j;
    vector<flowblock *> &exitlist = fd->bblocks.exitlist;
    vector<flowblock *> q;
    flowblock *b, *tmpb = NULL, *outb;

    blks.resize(fd->bblocks.get_size());

    j = blks.size() - 1;
    for (i = 0; i < exitlist.size(); i++) {
        b = exitlist[i];
        if (b->noreturn()) {
            tmpb = b->get_in(0);
            blks[j--] = b;
            b->set_mark();
            break;
        }
    }

    for (i = 0; i < exitlist.size(); i++) {
        b = exitlist[i];
        if (!b->noreturn()) {
            blks[j--] = b;
            b->set_mark();
        }
    }

    if (tmpb) {
        blks[j--] = tmpb;
        tmpb->set_mark();
    }

    q.push_back(fd->bblocks.get_entry_point());
    i = 0;
    while (!q.empty()) {
        b = q.front();
        q.erase(q.begin());

        if (b->is_mark()) continue;

        blks[i++] = b;
        b->set_mark();

        /* 某些simd指令会生成指令内相对跳转。需要把这几个cfg，全部排列在一起 */
        if (b->is_rel_header()) {
            tmpb = b->get_out(0);
            blks[i++] = tmpb;
            tmpb->set_mark();

            outb = tmpb->get_out(0)->is_rel_branch() ? tmpb->get_out(0) : tmpb->get_out(1);
            blks[i++] = outb;
            outb->set_mark();

            b = tmpb->get_cbranch_xor_out(outb);
            blks[i++] = b;
            b->set_mark();
        }

        for (j = 0; j < b->out.size(); j++) {
            outb = b->get_out(j);

            if (0 == outb->incoming_forward_branches()) {
                q.push_back(outb);
            }
        }
    }

    fd->clear_blocks_mark();
}