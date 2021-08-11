
#include "sleigh.hh"
#include "dobc.hh"
#include "thumb_gen.hh"

blockgraph::blockgraph(funcdata *fd1) 
{ 
    fd = fd1; 
    d = fd1->d;  
}

flowblock*  blockgraph::get_entry_point(void)
{
    int i;

    for (i = 0; i < blist.size(); i++) {
        if (blist[i]->is_entry_point())
            return blist[i];
    }

    return NULL;
}

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

void        blockgraph::calc_exitpath()
{
    flowblock *e, *in;
    int i, j;
    vector<flowblock *> q;

    for (i = 0; i < exitlist.size(); i++) {
        e = exitlist[i];

        e->flags.f_exitpath = 1;

        q.clear();
        q.push_back(e);

        while ((in = q.front())) {
            q.erase(q.begin());
            if (in->get_back_edge_count() > 0)
                continue;

            in->flags.f_exitpath = 1;

            for (j = 0; j < in->in.size(); j++) {
                q.push_back(in->get_in(j));
            }
        }
    }
}

void        blockgraph::clear(void)
{
    vector<flowblock *>::iterator iter;

    for (iter = blist.begin(); iter != blist.end(); ++iter)
        delete *iter;

    blist.clear();
}

void        blockgraph::clear_marks(void)
{
    int i;

    for (i = 0; i < blist.size(); i++)
        blist[i]->clear_mark();
}

int         blockgraph::remove_edge(flowblock *begin, flowblock *end)
{
    int i;
    for (i = 0; i < end->in.size(); i++) {
        if (end->in[i].point == begin)
            break;
    }

    return end->remove_in_edge(i);
}

void        blockgraph::add_edge(flowblock *begin, flowblock *end)
{
    end->add_in_edge(begin, 0);
}

void        blockgraph::add_edge(flowblock *begin, flowblock *end, int label)
{
    end->add_in_edge(begin, label);
}

void blockgraph::add_block(flowblock *b)
{
    int min = b->index;

    if (blist.empty())
        index = min;
    else {
        if (min < index) index = min;
    }

    b->parent = this;
    blist.push_back(b);
}

void blockgraph::find_spanning_tree(vector<flowblock *> &preorder, vector<flowblock *> &rootlist)
{
    if (blist.size() == 0) return;

    int i, origrootpos;
    vector<flowblock *> rpostorder;
    vector<flowblock *> state;
    int *visitcount;
    int rpostcount = blist.size();
    flowblock *tmpbl, *child;

    preorder.reserve(blist.size());
    rpostorder.resize(blist.size());
    visitcount = (int *)calloc(1, sizeof(int) * blist.size());

    exitlist.clear();
    clear_loopinfo();
    for (i = 0; i < blist.size(); i++) {
        tmpbl = blist[i];
        tmpbl->index = -1;
        tmpbl->dfnum = -1;
        tmpbl->copymap = tmpbl;
        if ((tmpbl->in.size() == 0))
            rootlist.push_back(tmpbl);

        if (!tmpbl->out.size())
            exitlist.push_back(tmpbl);

        tmpbl->clear_loopinfo();
    }
    if (rootlist.size() != 1) {
        throw LowlevelError("more root head");
    }

    origrootpos = rootlist.size() - 1;

    visitcount[0] = 0;
    state.push_back(blist[0]);
    preorder.push_back(blist[0]);
    blist[0]->dfnum = 0;

    /* 整个*/
    while (!state.empty()) {
        flowblock *bl = state.back();

        int index = visitcount[bl->dfnum];

        /* 当前节点的子节点都遍历完成 */
        if (index == bl->out.size()) {
            state.pop_back();
            bl->index = --rpostcount;
            rpostorder[rpostcount] = bl;
            if (!state.empty())
                state.back()->numdesc += bl->numdesc;
        }
        else {
            blockedge &e = bl->out[index];
            child = e.point;
            visitcount[bl->dfnum] += 1;

            /* */
            if (child->dfnum == -1) {
                bl->set_out_edge_flag(index, a_tree_edge);
                state.push_back(child);

                child->dfnum = preorder.size();
                /* dfs顺序的就是先序遍历 */
                preorder.push_back(child);
                visitcount[child->dfnum] = 0;
                child->numdesc = 1;
            }
            /* 假如发现out边上的节点指向的节点，是已经被访问过的，那么这是一条回边 */
            else if (child->index == -1) {
                bl->set_out_edge_flag(index, a_back_edge | a_loop_edge);
            }
            /**/
            else if (bl->dfnum < child->dfnum) {
                bl->set_out_edge_flag(index, a_forward_edge);
            }
            else
                bl->set_out_edge_flag(index, a_cross_edge);
        }
    }

    free(visitcount);
    blist = rpostorder;
}

/*
1. 找到不可规约边
2. 找到 spanning tree(计算df需要)
3. 设置flowblock的索引为反向支配顺序
4. 标记 tree-edge, forward-edges, cross-edges, 和 back-edge
    初步怀疑: tree-edge 是spanning tree
              forward-edge 是
*/
void blockgraph::structure_loops(vector<flowblock *> &rootlist)
{
    vector<flowblock *> preorder;
    int irreduciblecount = 0;

    find_spanning_tree(preorder, rootlist);
    /* vm360的图是不可规约的，还不确认不可规约的图会对优化造成什么影响 */
    find_irreducible(preorder, irreduciblecount);
}

void blockgraph::dump_spanning_tree(const char *filename, vector<flowblock *> &rootlist)
{
    FILE *fp;
    int i;

    fp = fopen(filename, "w");

    fprintf(fp, "digraph G {\n");
    fprintf(fp, "node [fontname = \"helvetica\"]\n");

    for (i = 0; i < rootlist.size(); i++) {
    }

    fclose(fp);
}

/*

paper: A Simple, Fast Dominance Algorithm
http://web.cse.ohio-state.edu/~rountev.1/788/papers/cooper-spe01.pdf
*/
void  blockgraph::calc_forward_dominator(const vector<flowblock *> &rootlist)
{
    vector<flowblock *>     postorder;
    flowblock *b, *new_idom, *rho;
    bool changed;
    int i, j, finger1, finger2;

    if (blist.empty())
        return;

    if (rootlist.size() > 1)
        throw LowlevelError("we are not support rootlist.size() exceed 1");

    int numnodes = blist.size() - 1;
    postorder.resize(blist.size());
    for (i = 0; i < blist.size(); i++) {
        blist[i]->immed_dom = NULL;
        postorder[numnodes - i] = blist[i];
    }

    b = postorder.back();
    if (b->in.size()) {
        throw LowlevelError("entry node in edge error");
    }

    b->immed_dom = b;
    for (i = 0; i < b->out.size(); i++)
        b->get_out(i)->immed_dom = b;

    changed = true;
    new_idom = NULL;

    while (changed) {
        changed = false;
        for (i = postorder.size() - 2; i >= 0; --i) {
            b = postorder[i];

            /* 感觉这个判断条件是不需要的，但是Ghdira源代码里有 */
            if (b->immed_dom == postorder.back()) {
                continue;
            }

            for (j = 0; j < b->in.size(); j++) {
                new_idom = b->get_in(j);
                if (new_idom->immed_dom)
                    break;
            }

            j += 1;
            for (; j < b->in.size(); j++) {
                rho = b->get_in(j);
                if (rho->immed_dom) {
                    finger1 = numnodes - rho->index;
                    finger2 = numnodes - new_idom->index;
                    while (finger1 != finger2) {
                        while (finger1 < finger2)
                            finger1 = numnodes - postorder[finger1]->immed_dom->index;
                        while (finger2 < finger1)
                            finger2 = numnodes - postorder[finger2]->immed_dom->index;
                    }
                    new_idom = postorder[finger1];
                }
            }
            if (b->immed_dom != new_idom) {
                b->immed_dom = new_idom;
                changed = true;
            }
        }
    }

    postorder.back()->immed_dom = NULL;
}

void blockgraph::post_order_on_rtree(flowblock *root, vector<flowblock *> &postorder, vector<char> &mark)
{
    int i;

    if (mark[root->index]) return;
    mark[root->index] = 1;

    for (i = 0; i < root->in.size(); i++)
        post_order_on_rtree(root->get_in(i), postorder, mark);

    root->rindex = postorder.size();
    postorder.push_back(root);
}

void   blockgraph::add_exit()
{
    int i;

    for (i = 0; i < exitlist.size(); i++)
        add_edge(exitlist[i], &exit);
}

void    blockgraph::del_exit()
{
    int i;
    for (i = 0; i < exitlist.size(); i++)
        remove_edge(exitlist[i], &exit);
}

void  blockgraph::calc_post_dominator()
{
    vector<flowblock *>     postorder;
    vector<char> mark;
    flowblock *b, *new_idom, *rho;
    bool changed;
    int i, j, finger1, finger2;

    add_exit();

    mark.resize(blist.size() + 1);
    post_order_on_rtree(&exit, postorder, mark);

    /* 由于我们的图中，默认没有统一的出口节点，所以不需要计算 */
    int numnodes = postorder.size();

    for (i = 0; i < postorder.size(); i++)
        postorder[i]->post_immed_dom = NULL;

    b = postorder.back();
    b->post_immed_dom = &exit;

    for (i = 0; i < b->in.size(); i++)
        b->get_in(i)->post_immed_dom = b;

    changed = true;
    new_idom = NULL;

    while (changed) {
        changed = false;
        for (i = postorder.size() - 2; i >= 0; --i) {
            b = postorder[i];

            /* 感觉这个判断条件是不需要的，但是Ghdira源代码里有 */
            if (b->post_immed_dom == postorder.back()) 
                continue;

            for (j = 0; j < b->out.size(); j++) {
                new_idom = b->get_out(j);
                if (new_idom->post_immed_dom)
                    break;
            }

            j += 1;
            for (; j < b->out.size(); j++) {
                rho = b->get_out(j);
                if (rho->post_immed_dom) {
                    finger1 = rho->rindex;
                    finger2 = new_idom->rindex;
                    while (finger1 != finger2) {
                        while (finger1 < finger2)
                            finger1 = postorder[finger1]->post_immed_dom->rindex;
                        while (finger2 < finger1)
                            finger2 = postorder[finger2]->post_immed_dom->rindex;
                    }
                    new_idom = postorder[finger1];
                }
            }
            if (b->post_immed_dom!= new_idom) {
                b->post_immed_dom= new_idom;
                changed = true;
            }
        }
    }

    postorder.back()->post_immed_dom = NULL;
    del_exit();
}

flowblock* blockgraph::new_block_basic(void)
{
    flowblock *ret = new flowblock(fd);
    add_block(ret);
    return ret;
}

flowblock* blockgraph::new_block_basic(intb offset)
{
    flowblock *b = new_block_basic();

    Address addr(fd->d->getDefaultCodeSpace(), offset);
    b->set_initial_range(addr, addr);

    return b;
}

void        blockgraph::set_start_block(flowblock *bl)
{
    int i;
    if (blist[0]->flags.f_entry_point) {
        if (bl == blist[0]) return;
    }

    for (i = 0; i < blist.size(); i++)
        if (blist[i] == bl) break;

    for (; i > 0; --i)
        blist[i] = blist[i - 1];

    blist[0] = bl;
    bl->flags.f_entry_point = 1;
}



void        blockgraph::remove_block(flowblock *bl)
{
    vector<flowblock *>::iterator iter;

    if (bl->in.size())
        throw LowlevelError("only support remove block in-size is 0");

    bl->flags.f_dead = 1;

    for (iter = blist.begin(); iter != blist.end(); iter++) {
        if (*iter == bl) {
            blist.erase(iter);
            break;
        }
    }

    deadblist.push_back(bl);
}

void        blockgraph::collect_reachable(vector<flowblock *> &res, flowblock *bl, bool un) const
{
    flowblock *blk, *blk2;

    bl->set_mark();
    res.push_back(bl);
    int total = 0, i, j;

    while (total < res.size()) {
        blk = res[total++];
        for (j = 0; j < blk->out.size(); j++) {
            blk2 = blk->get_out(j);
            if (blk2->is_mark())
                continue;

            blk2->set_mark();
            res.push_back(blk2);
        }
    }

    if (un) {
        res.clear();
        for (i = 0; i < blist.size(); i++) {
            blk = blist[i];
            if (blk->is_mark())
                blk->clear_mark();
            else
                res.push_back(blk);
        }
    }
    else {
        for (i = 0; i < res.size(); i++)
            res[i]->clear_mark();
    }
}

void        blockgraph::splice_block(flowblock *bl)
{
    flowblock *outbl = (flowblock *)0;
    char f[3];
    if (bl->out.size() == 1) {
        outbl = bl->get_out(0);
        if (outbl->in.size() != 1)
            outbl = NULL;
    }
    if (outbl == NULL)
        throw LowlevelError("Can only splice block with 1 output to ");

    f[0] = bl->flags.f_entry_point;
    bl->remove_out_edge(0);

    int sizeout = outbl->out.size();
    for (int i = 0; i < sizeout; i++) 
        move_out_edge(outbl, 0, bl);

    remove_block(outbl);
    bl->flags.f_entry_point = f[0];
}

void        blockgraph::move_out_edge(flowblock *blold, int slot, flowblock *blnew)
{
    flowblock *outbl = blold->get_out(slot);
    int i = blold->get_out_rev_index(slot);
    outbl->replace_in_edge(i, blnew);
}

flowblock*  blockgraph::add_block_if(flowblock *b, flowblock *cond, flowblock *tc)
{
    add_edge(b, cond, a_true_edge);
    add_edge(b, tc);

    return b;
}

bool        blockgraph::is_dowhile(flowblock *b)
{
    int i;

    for (i = 0; i < b->out.size(); i++) {
        flowblock *o = b->get_out(i);

        if (o == b)
            return true;
    }

    return false;
}

pcodeop*    blockgraph::first_callop_vmp(flowblock *end)
{
    list<pcodeop *>::iterator it;
    pcodeop *op;
    dobc *d = fd->d;

    for (int i = 0; i < blist.size(); i++) {
        flowblock *b = blist[i];

        if (b == end)
            return NULL;

        for (it = b->ops.begin(); it != b->ops.end(); it++) {
            op = *it;
            if (op->is_call() && d->test_cond_inline(d, op->get_call_offset()))
                return op;
        }
    }

    return NULL;
}

flowblock*  blockgraph::find_loop_exit(flowblock *start, flowblock *end)
{
    vector<flowblock *> stack;
    vector<int> visit;
    flowblock *b, *bb;
    int i;

    visit.resize(get_size());

    stack.push_back(start);
    visit[start->index] = 1;

    while (!stack.empty()) {
        b = stack.back();

        if (b == end) {
            do {
                stack.pop_back();
                b = stack.back();
            } while (b->out.size() == 1);

            return b;
        }

        for (i = 0; i < b->out.size(); i++) {
            bb = b->get_out(i);
            if (visit[bb->index]) continue;

            visit[bb->index] = 1;
            stack.push_back(bb);
            break;
        }

        if (i == b->out.size()) stack.pop_back();
    }

    return NULL;
}

flowblock*          blockgraph::detect_whiledo_exit(flowblock *header)
{
    flowblock *true0, *false0, *back;

    if (header->out.size() != 2)
        return NULL;

    if (header->get_back_edge_count() == 0)
        return NULL;

    back = header->get_back_edge_node();

    true0 = header->get_true_edge()->point;
    false0 = header->get_false_edge()->point;

    while (back) {
        if (back == true0) return false0;
        if (back == false0) return true0;

        back = back->immed_dom;
    }

    throw LowlevelError("loop is unreducible ?");
}



flowblock::flowblock(void)
{
}

flowblock::flowblock(funcdata *f)
{
    fd = f;
}

flowblock::~flowblock()
{
}

void        flowblock::set_initial_range(const Address &b, const Address &e)
{
    cover.clear();
    cover.insertRange(b.getSpace(), b.getOffset(), e.getOffset());
}

bool        flowblock::is_empty(int except_branch)
{
    pcodeop *op;
    list<pcodeop *>::iterator it;

    if (out.size() == 1 && get_out(0) == this) return false;

    if (ops.empty()) return true;

    for (it = ops.begin(); it != ops.end(); it++) {
        op = *it;
        if ((op->opcode == CPUI_MULTIEQUAL)) continue;
        if (except_branch && (op->opcode == CPUI_BRANCH)) continue;

        return false;
    }

    return true;
}

bool        flowblock::is_empty_delete(void)
{
    if (out.size() != 1) return false;
    if (get_out(0) == this) return false;

    return is_empty(0);
}

void        flowblock::insert(list<pcodeop *>::iterator iter, pcodeop *inst)
{
    list<pcodeop *>::iterator newiter;
    inst->parent = this;
    newiter = ops.insert(iter, inst);
    inst->basiciter = newiter;
}

int         flowblock::sub_id() 
{ 
    return index;
    if (ops.size() == 0) return 0;

    list<pcodeop *>::const_iterator iter = ops.begin();
    return (*iter)->start.getTime();
}

bool        flowblock::noreturn(void) 
{ 
    return ops.size() && last_op()->callfd && last_op()->callfd->noreturn();
}

pcodeop*    flowblock::get_cbranch_sub_from_cmp(void)
{
    pcodeop *lastop = last_op(), *op;
    vector<pcodeop *> q;
    varnode *in0, *in1;
    if (NULL == lastop || (lastop->opcode != CPUI_CBRANCH)) return NULL;

    op = lastop->get_in(1)->def;
    q.push_back(op);

    while (!q.empty()) {
        op = q.front();
        q.erase(q.begin());

        while (op && op->parent == this) {
            switch (op->opcode) {
            case CPUI_COPY:
            case CPUI_BOOL_NEGATE:
                op = op->get_in(0)->def;
                break;

            case CPUI_INT_NOTEQUAL:
            case CPUI_INT_EQUAL:
            case CPUI_INT_SLESS:
            case CPUI_BOOL_OR:
            case CPUI_BOOL_AND:
                in0 = op->get_in(0);
                in1 = op->get_in(1);
                if (in0->def) {
                    op = in0->def;

                    if (in1->def)
                        q.push_back(in1->def);
                }
                else
                    op = in1->def;
                break;

            case CPUI_INT_SUB:
                return op;

            default:
                op = NULL;
                break;
            }
        }
    }

    return NULL;
}

pcodeop*    flowblock::get_last_oper_zr(void)
{
    list<pcodeop *>::iterator it = ops.end();
    dobc *d = fd->d;

    do {
        pcodeop *p = *--it;
        if (p->output && poa(p) == d->zr_addr) return p;
    } while (it != ops.begin());

    return NULL;
}

int         flowblock::get_cbranch_cond()
{
    list<pcodeop *>::iterator it = find_inst_first_op(*--ops.end());
    dobc *d = fd->d;

    /* 这里要全部重写，这样一个个if pattern识别过去，我会累死的
    可能会改成公共前缀的某种trie树来做。 */
    if ((*it)->opcode == CPUI_INT_EQUAL
        && (*it)->get_in(0)->get_addr() == d->zr_addr
        && (*it)->get_in(1)->is_constant()
        && (*it)->get_in(1)->get_val() == 0
        && (*++it)->opcode == CPUI_BOOL_NEGATE
        && (*++it)->opcode == CPUI_CBRANCH)
        return COND_EQ;

    return -1;
}

bool        flowblock::have_same_cmp_condition(flowblock *b)
{
    list<pcodeop *>::iterator it1, it2;

    it1 = find_inst_first_op(get_cbranch_sub_from_cmp());
    it2 = find_inst_first_op(b->get_cbranch_sub_from_cmp());

    for (; it1 != ops.end() && it2 != ops.end(); it1++, it2++) {
        if ((*it1)->opcode == CPUI_CBRANCH && (*it2)->opcode == CPUI_CBRANCH) return true;
        if (pcodeop_struct_cmp(*it1, *it2, 1)) return false;
    }

    return false;
}

int         flowblock::lead_to_edge(pcodeop *op, pcodeop *phi, varnode *vn)
{
    list<pcodeop *>::iterator it = op->basiciter;
    flowblock *branch = NULL;
    map<pcodeop *, valuetype, pcodeop_cmp> opmap;
    map<pcodeop *, valuetype, pcodeop_cmp>::iterator oit;
    int ret;

    valuetype save = phi->output->type;

    phi->output->type = vn->type;

    for (; it != ops.end(); it++) {
        pcodeop *p = *it;

        ret = fd->static_trace(p, -1, &branch);
    }

    fd->static_trace_restore();

    phi->output->type = save;

    if (ret != ERR_MEET_CALC_BRANCH) return -1;

    return (get_false_edge()->point == branch) ? 0 : 1;
}

bool        flowblock::lead_to_false_edge(pcodeop *op, pcodeop *phi, varnode *vn)
{
    return (lead_to_edge(op, phi, vn) == 0) ? true : false;
}

bool        flowblock::lead_to_true_edge(pcodeop *op, pcodeop *phi, varnode *vn)
{
    return (lead_to_edge(op, phi, vn) == 1) ? true : false;
}

list<pcodeop*>::iterator    flowblock::find_inst_first_op(pcodeop *p)
{
    if (p == NULL) return ops.end();

    list<pcodeop *>::iterator it = p->basiciter;
    const Address &addr = p->get_addr();

    for (; (*it)->get_addr() == addr; it--) {
        if (it == ops.begin()) return it;
    }

    return ++it;
}

int         flowblock::calc_memflow()
{
    varnode *in1, *in2;
    dobc *d = fd->d;
    list<pcodeop *>::iterator it;
    flowblock *inb;
    /* 和一般的数据流计算方式一样 
    
    */

    flowblock *min;
    if (in.size()) {
        /* 计算自己的前驱节点的交集 */
        min = get_in(0);
        for (int i = 1; i < in.size(); i++) {
            inb = get_in(i);

            if (inb->memflow.livein.size() < min->memflow.livein.size())
                min = inb;
        }

        set<varnode *>::iterator mit = min->memflow.livein.begin();

        for (; mit != min->memflow.livein.end(); mit++) {
            for (int i = 0; i < in.size(); i++) {
                inb = get_in(i);
                if (inb == min) continue;
            }
        }
    }

    for (it = sideeffect_ops.begin(); it != sideeffect_ops.end(); it++) {
        pcodeop *p = *it;

        if (p->is_call()) {
            /* 汇编里的call指令会导致所有的内存访问都变成may store，除非你能知道这个call
            究竟store了多少位置 */
            memflow.liveout.clear();
            memflow.have_call = 1;
            continue;
        }

        in1 = p->get_in(1);
        in2 = p->get_in(2);

        if (p->opcode == CPUI_LOAD) {
            if (!memflow.have_call && !memflow.have_may_store) {
                if (in2->is_sp_constant()) {
                }
            }
        }
        else if (p->opcode == CPUI_STORE) {
            if (in1->is_top()) {
                memflow.have_may_store = 1;
                memflow.liveout.clear();
                continue;
            }

            if (!p->output && in1->is_sp_constant()) {
                Address addr(d->getStackBaseSpace(), in1->get_val());
                p->output = fd->create_vn(in2->size, addr);
            }

            /* 只有virtual stack上的内存访问才有output */
            if (p->output) {
                memflow.liveout.insert(p->output);
            }
        }
    }

    return 1;
}


varnode*    flowblock::get_false_vn(pcodeop *phi)
{
    if (!is_cbranch()) return NULL;

    flowblock *b = phi->parent;

    for (int i = 0; i < out.size(); i++) {
        blockedge *e = &out[i];

        if (!(e->label & a_true_edge)) {
            if (e->point == phi->parent)
                return phi->get_in(e->reverse_index);
            else {
                e = &e->point->out[0];
                return phi->get_in(e->reverse_index);
            }
        }
    }

    return NULL;
}

/*
*/
varnode*    flowblock::get_true_vn(pcodeop *phi)
{
    if (!is_cbranch()) return NULL;

    flowblock *b = phi->parent;

    for (int i = 0; i < out.size(); i++) {
        blockedge *e = &out[i];

        if (e->label & a_true_edge) {
            if (e->point == phi->parent)
                return phi->get_in(e->reverse_index);
            else {
                e = &e->point->out[0];
                return phi->get_in(e->reverse_index);
            }
        }
    }

    return NULL;
}

int         flowblock::incoming_forward_branches()
{
    int i, count = 0;

    for (i = 0; i < in.size(); i++) {
        blockedge &e = in[i];
        if (!(e.label & a_forward_edge)) continue;
        if (e.point->is_mark()) continue;

        count++;
    }

    return count;
}

bool        flowblock::is_iv_in_normal_loop(pcodeop *sub)
{
    varnode *in1 = sub->get_in(1);

    if (in1->is_constant() && (in1->get_val() >= 0) && in1->get_val() <= 1024)
        return true;

    return false;
}

bool        flowblock::is_eq_cbranch(void)
{
    pcodeop *lastop = last_op(), *def;
    if (lastop && (lastop->opcode == CPUI_CBRANCH) && (def = lastop->get_in(1)->def)
        && (def->opcode == CPUI_INT_NOTEQUAL)
        && (def->get_in(0)->get_addr() == dobc::singleton()->get_addr("ZR"))
        && def->get_in(1)->is_constant()
        && (def->get_in(1)->get_val() == 0))
        return true;

    return false;
}

int         flowblock::get_in_index(const flowblock *bl)
{
    int i;

    for (i = 0; i < in.size(); i++) {
        if (in[i].point == bl)
            return i;
    }

    return -1;
}

int         flowblock::get_out_index(const flowblock *bl)
{
    int i;

    for (i = 0; i < out.size(); i++) {
        if (out[i].point == bl)
            return i;
    }

    return -1;
}

void        flowblock::add_in_edge(flowblock *b, int lab)
{
    int outrev = b->out.size();
    int brev = in.size();
    in.push_back(blockedge(b, lab, outrev));
    b->out.push_back(blockedge(this, lab, brev));
}

int         flowblock::remove_in_edge(int slot)
{
    flowblock *b = in[slot].point;
    int label = in[slot].label;
    int rev = in[slot].reverse_index;

    half_delete_in_edge(slot);
    b->half_delete_out_edge(rev);

    return label;
}

void        flowblock::remove_out_edge(int slot)
{
    flowblock *b = out[slot].point;
    int rev = out[slot].reverse_index;

    half_delete_out_edge(slot);
    b->half_delete_in_edge(rev);
}

void        flowblock::half_delete_out_edge(int slot)
{
    while (slot < (out.size() - 1)) {
        blockedge &edge(out[slot]);
        edge = out[slot + 1];

        blockedge &edge2(edge.point->in[edge.reverse_index]);
        edge2.reverse_index -= 1;
        slot += 1;
    }

    out.pop_back();
}

void        flowblock::half_delete_in_edge(int slot)
{
    while (slot < (in.size() - 1)) {
        blockedge &edge(in[slot]);
        edge = in[slot + 1];

        blockedge &edge2(edge.point->out[edge.reverse_index]);
        edge2.reverse_index -= 1;
        slot += 1;
    }
    in.pop_back();
}

int         flowblock::get_back_edge_count(void)
{
    int i, count = 0;

    for (i = 0; i < in.size(); i++) {
        if (in[i].label & a_back_edge) count++;
    }

    return count;
}

flowblock*  flowblock::get_back_edge_node(void)
{
    int i, count = 0;

    for (i = 0; i < in.size(); i++) {
        if (in[i].label & a_back_edge) return in[i].point;
    }

    return NULL;
}

blockedge* flowblock::get_true_edge(void)
{
    pcodeop *op = last_op();
    int i;

    if (op->opcode != CPUI_CBRANCH)
        throw LowlevelError("get_true_edge() only support CPUI_CBRANCH");


    for (i = 0; i < out.size(); i++) {
        if (out[i].label & a_true_edge)
            return &out[i];
    }

    fd->dump_cfg(fd->name, "check000", 1);
    throw LowlevelError("not found true edge in flowblock");
}

blockedge*  flowblock::get_false_edge(void)
{
    pcodeop *op = last_op();
    int i;

    if (op->opcode != CPUI_CBRANCH)
        throw LowlevelError("get_false_addr() only support CPUI_CBRANCH");

    for (i = 0; i < out.size(); i++) {
        if (!(out[i].label & a_true_edge))
            return &out[i];
    }

    throw LowlevelError("not found false edge in flowblock");
}

void        flowblock::set_out_edge_flag(int i, uint4 lab)
{
    flowblock *bbout = out[i].point;
    out[i].label |= lab;
    bbout->in[out[i].reverse_index].label |= lab;
}

void        flowblock::clear_out_edge_flag(int i, uint4 lab)
{
    flowblock *bbout = out[i].point;
    out[i].label &= ~lab;
    bbout->in[out[i].reverse_index].label &= ~lab;
}

void        blockgraph::remove_from_flow(flowblock *bl)
{
    if (bl->in.size() > 0)
        throw LowlevelError("only support remove block which in-size is 0");

    flowblock *bbout;

    while (bl->out.size() > 0) {
        bbout = bl->get_out(bl->out.size() - 1);
        bl->remove_out_edge(bl->out.size() - 1);
    }
}

void        flowblock::remove_op(pcodeop *inst)
{
    inst->parent = NULL;
    ops.erase(inst->basiciter);
}

