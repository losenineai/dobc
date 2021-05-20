
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
    flowblock *b;

    blks.resize(fd->bblocks.get_size());

    q.push_back(fd->bblocks.get_entry_point());
    i = 0;
    while (q.empty()) {
        b = q.front();
        q.erase(q.begin());

        blks[i++] = b;

        b->set_mark();

        for (j = 0; j < b->out.size(); j++) {
            flowblock *outb = b->get_out(j);

            if (0 == outb->incoming_forward_branches())
                q.push_back(outb);
        }
    }

    fd->clear_blocks_mark();
}