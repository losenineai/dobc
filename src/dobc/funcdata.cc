
#include "sleigh.hh"
#include "dobc.hh"
#include <assert.h>
#include "vm.h"

int  funcdata::loop_dfa_connect(uint32_t flags)
{
    int i, inslot, ret, end, nend;
    flowblock *cur, *prev, *br,  *last, *h = ollvm_get_head(), *from;
    list<pcodeop *>::const_iterator it;
    pcodeop *p, *op;
    varnode *iv = NULL;
    blockedge *chain = NULL;

    if (ollvm_detect_propchains(from, chain)) {
        printf("ollvm not found propchain\n");
        return -1;
    }

    prev = from;
    cur = chain->point;

    prev->dump();
    cur->dump();

    trace_push(prev->last_op());

    do {
        printf("\tprocess flowblock sub_%llx\n", cur->get_start().getOffset());

        it = cur->ops.begin();
        inslot = cur->get_inslot(prev);

        for (ret = 0; it != cur->ops.end(); it++) {
            p = *it;

            br = NULL;
            p->set_trace();
            ret = p->compute(inslot, &br);

            if (flags & _DUMP_PCODE) {
                char buf[256];
                p->dump(buf, PCODE_DUMP_SIMPLE & ~PCODE_HTML_COLOR);
                printf("%s\n", buf);
            }

            trace_push(p);
        }

        if ((cur->out.size() > 1) && (ret != ERR_MEET_CALC_BRANCH)) {
            printf("found undefined-bcond in block[%x]\n", cur->sub_id());

            for (end = trace.size() - 1; trace[end]->parent == cur; end--);

            /* 假如trace的最后一个节点，直接等于from，那么证明，一个分支节点都没走过去，直接禁止掉这个edge */
            if (trace[end]->parent == from) {
                printf("warn: only one block, forbidden this edge\n");
                chain->set_flag(a_unpropchain);

                for (i = 0; i < trace.size(); i++) {
                    pcodeop *p = trace[i];
                    p->clear_trace();
                    p->set_top();
                }

                heritage_clear();
                heritage();

                return 1;
            }

            break;
        }

        if ((cur->ops.size() == 0)) {
            assert(cur->out.size() == 1);
            br = cur->get_out(0);
        }

        prev = cur;
        cur = br;
        end = trace.size() - 1;
    /* 有2种情况会退出循环，
    
    1. 走出循环以后
    2. 碰到无法识别的cbranch节点 */
    } while (!cur->flags.f_exitpath);

    if (end == trace.size() - 1)
        printf("found exit node[%lx]\n", cur->sub_id());

    /* back指向的节点
    1. 当走出循环时，back指向循环外节点
    2. 当碰到undefined cbranch, trace.back指向了第一个无法计算的循环节点 */
    last = (end == trace.size() - 1) ? cur:trace.back()->parent;
    cur = bblocks.new_block_basic();

    user_offset += user_step;
    Address addr(d->get_code_space(), user_offset);
    /* 进入节点抛弃 */
    for (i = 0; trace[i]->parent == from; i++);

    /* 我们在复制节点的时候，末尾只有单个out节点的cfg不理会，这个是为了减少这种情况
    a是个cbranch节点，在a分叉
    a-> b -> c -> d
    a-> e -> c -> d
    当我们顺着a节点复制时，假如不在c节点停止，会复制出2条路径出来
    */
    for (nend = end; trace[nend]->parent->out.size() == 1; nend--);

    if (nend != end) {
        last = trace[nend + 1]->parent;
        end = nend;
    }

    /* 从主循环开始 */
    for (; i <= end; i++) {
        funcdata *callfd = NULL;
        p = trace[i];

        if ((p->opcode == CPUI_CALLIND) && p->get_in(0)->is_constant()) {
            Address addr(d->get_code_space(), p->get_in(0)->get_val());
            callfd = d->find_func(addr);
        }

        /* 删除所有中间的 跳转指令和phi节点 */
        if (((p->opcode == CPUI_BRANCH) || (p->opcode == CPUI_CBRANCH) || (p->opcode == CPUI_INDIRECT) || (p->opcode == CPUI_MULTIEQUAL) || (p->opcode == CPUI_BRANCHIND)))
            continue;

        Address addr2(d->get_code_space(), user_offset + p->get_addr().getOffset());
        const SeqNum sq(addr2, op_uniqid++);
        op = cloneop(p, sq);
        op_insert(op, cur, cur->ops.end());

        /* 假如trace以后，发现某个函数可以被计算，把他加入trace列表 */
        if (callfd && !op->callfd) {
            add_callspec(op, callfd);
        }
    }

    for (i = 0; i < trace.size(); i++) {
        pcodeop *p = trace[i];
        p->clear_trace();
        p->set_top();
    }

    cur->set_initial_range(addr, addr);
    trace_clear();

    clear_block_phi(chain->point);

    int lab = bblocks.remove_edge(from, chain->point);
    bblocks.add_edge(from, cur, lab & a_true_edge);
    bblocks.add_edge(cur, last);

    cond_constant_propagation();

    if (!cbrlist.empty())
        cond_constant_propagation();

    return 0;
}

int         funcdata::collect_blocks_to_node(vector<flowblock *> &blks, flowblock *start, flowblock *end)
{
    vector<flowblock *> stack;
    vector<int>         visit;
    flowblock *b, *out;
    int i;

    visit.resize(bblocks.get_size());
    stack.push_back(start);
    visit[start->index] = 1;

    while (!stack.empty()) {
        b = stack.back();

        for (i = 0; i < b->out.size(); i++) {
            out = b->get_out(i);
            if (out == end) {
                if (!b->is_mark()) {
                    b->set_mark();
                    blks.push_back(b);
                }
                continue;
            }

            if (!visit[out->index]) {
                visit[out->index] = 1;
                stack.push_back(out);
                break;
            }
        }

        if (i == b->out.size()) stack.pop_back();
    }

    for (i = 0; i < blks.size(); i++) {
        blks[i]->clear_mark();
    }

    return 0;
}

void        funcdata::dead_code_elimination(vector<flowblock *> blks, uint32_t flags)
{
    flowblock *b;
    list<pcodeop *>::iterator it;
    list<pcodeop *> worklist;
    vector<flowblock *> marks;
    pcodeop *op;
    int i;

    marks.clear();
    marks.resize(bblocks.get_size());

    for (i = blks.size() - 1; i >= 0; i--) {
        b = blks[i];

        marks[b->dfnum] = b;

        for (it = b->ops.begin(); it != b->ops.end(); it++) {
            op = *it;
            if (op->output && op->output->has_no_use()) {
                worklist.push_back(op);
            }
        }
    }

    while (!worklist.empty()) {
        it = worklist.begin();
        op = *it;
        worklist.erase(it);

        if (op->flags.dead) continue;
        //printf("delete pcode = %d\n", op->start.getTime());

        if (!op->output->has_no_use()) continue;
        /*
        FIXME:暂时不允许删除store命令

        store在某些情况下被赋予了 virtual节点以后，会和load关联起来
        并生成了一个虚拟out节点，当这个节点没人使用时，也不允许删除
        store只有一种情况下可以删除，就是

        1. mem[x] = 1;
        2. mem[x] = 2;

        对同一地址的写入会导致上一个写入失效，这中间不能有上一个地址的访问
        */

        if (op->opcode == CPUI_STORE) continue;
        /* 有些函数是有副作用的，它的def即使没有use也是不能删除的 */
        if (op->is_call()) continue;

        pcodeop_def_set::iterator it1 = topname.find(op);
        if ((it1 != topname.end()) && (*it1 == op))
            continue;

        for (int i = 0; i < op->num_input(); i++) {
            varnode *in = op->get_in(i);
            /* 有些varnode节点是常量传播出来得常量，有些天然常量 */
            if (in->get_addr().isConstant()) continue;
            /* 输入节点是没有def的 */
            if (in->is_input()) continue;
            if (!in->def) continue;
            worklist.push_back(in->def);
        }

        if (marks[op->parent->dfnum]) 
            op_destroy(op, 1);
    }

    flowblock *h;

    if (flags & RDS_0) 
        remove_dead_store(bblocks.get_block(0));

    if ((flags & RDS_UNROLL0) && (h = get_vmhead_unroll())) 
        remove_dead_store(h);
}

bool        funcdata::is_code(varnode *v0, varnode *v1) 
{ 
    if ((v0->loc.getSpace()->getType() == IPTR_CONSTANT) && (v0->loc.getOffset() == (uintb)(d->trans->getDefaultCodeSpace())) && v1->is_constant()) {

        /* FIXME: hardcode 直接编码了libjiagu.so 的 bss段位置，后面要去掉，从ida中获取到的 */
        if ((v1->get_val() >= 0x855dc) && (v1->get_val() < 0x8576c))
            return false;

        return true;
    }

    return false;
}

bool        funcdata::is_sp_rel_constant(varnode *v)
{
    return v->is_rel_constant() && (v->get_rel() == d->sp_addr);
}

void        funcdata::place_multiequal(void)
{
    LocationMap::iterator iter;
    vector<varnode *> readvars;
    vector<varnode *> writevars;
    vector<varnode *> inputvars;
    pcodeop *multiop, *p;
    varnode *vnin;
    blockbasic *bl;
    int max, i, j;

    for (iter = disjoint.begin(); iter != disjoint.end(); ++iter) {
        Address addr = (*iter).first;
        int size = (*iter).second.size;

        readvars.clear();
        writevars.clear();
        inputvars.clear();
        max = collect(addr, size, readvars, writevars, inputvars);
        if ((size > 4) && (max < size)) {
        }

        /* FIXME:后面那个判断我没看懂，抄Ghidra的 */
        if (readvars.empty() && (addr.getSpace()->getType() == IPTR_INTERNAL))
            continue;

        if (readvars.empty() && writevars.empty())
            continue;

        if (!d->is_cpu_reg(addr))
            continue;

        calc_phi_placement2(writevars);
        for (i = 0; i < merge.size(); ++i) {
            bl = merge[i];

            list<pcodeop *>::iterator it = bl->ops.begin();
            varnode *vnout = NULL;

            for (multiop = NULL; it != bl->ops.end(); it++) {
                p = *it;

                if ((p->opcode != CPUI_MULTIEQUAL) && !p->flags.copy_from_phi) break;
                if (p->output->get_addr() == addr) {
                    multiop = p;
                    break;
                }
            }

            /* 假如说某个PHI节点已经被转成了copy节点，则说明这个值已经被常量化，这个节点在下次heritage
            时已经被需要在插入phi 了。 
            FIXME:这个理解是否正确？*/
            if ((it != bl->ops.end()) && p->flags.copy_from_phi) continue;

            if (!multiop) {
                multiop = newop(bl->in.size(), bl->get_start());
                vnout = new_varnode_out(size, addr, multiop);
                op_set_opcode(multiop, CPUI_MULTIEQUAL);
                op_insert_begin(multiop, bl);
                j = 0;
            }
            else {
                /* 假如已经有个从phi节点转成的copy节点，删除它的输入节点 */
                if (multiop->opcode == CPUI_COPY) {
                    while (multiop->num_input() > 0)
                        op_remove_input(multiop, 0);

                    op_set_opcode(multiop, CPUI_MULTIEQUAL);
                    multiop->flags.copy_from_phi = 0;
                }

                j = multiop->num_input();
            }

            for (; j < bl->in.size(); j++) {
                vnin = new_varnode(size, addr);
                op_set_input(multiop, vnin, j);
            }
        }
    }

    merge.clear();
}

void        funcdata::rename()
{
    variable_stack varstack;
    version_map vermap;

    rename_recurse(bblocks.get_block(0), varstack, vermap);

    disjoint.clear();
}

void        funcdata::rename_recurse(blockbasic *bl, variable_stack &varstack, version_map &vermap)
{
    /* 当前block内，被def过得varnode集合 */
    vector<varnode *> writelist;
    blockbasic *subbl;
    list<pcodeop *>::iterator oiter, suboiter, next;
    pcodeop *op, *multiop;
    varnode *vnout, *vnin, *vnnew;
    int i, slot, set_begin = 0, order;

    for (oiter = bl->ops.begin(), order = 0; oiter != bl->ops.end(); oiter = next, order++) {
        op = *oiter ;
		next = ++oiter;
        op->start.setOrder(order);

        if (op->opcode != CPUI_MULTIEQUAL) {
            if ((op->opcode == CPUI_COPY) && (op->output->get_addr() == op->get_in(0)->get_addr())) {
                op_destroy_ssa(op);
				order--;
                continue;
            }

            for (slot = 0; slot < op->inrefs.size(); ++slot) {
                vnin = op->get_in(slot);

                if (vnin->flags.annotation || (vnin->is_constant() && vnin->get_addr().isConstant()))
                    continue;

                vector<varnode *> &stack(varstack[vnin->get_addr()]);
                if (stack.empty()) {
                    vnnew = new_varnode(vnin->size, vnin->get_addr());
                    vnnew = set_input_varnode(vnnew);
                    vnnew->version = vermap[vnin->get_addr()];
                    stack.push_back(vnnew);
                }
                else 
                    vnnew = stack.back();

				vnnew->add_ref_point_simple(op);

                op_set_input(op, vnnew, slot);
                if (vnin->has_no_use()) {
                    delete_varnode(vnin);
                }
            }
        }

        vnout = op->output;
        if (vnout == NULL) continue;
        vnout->version = ++vermap[vnout->get_addr()];

		vector<varnode *> &stack(varstack[vnout->get_addr()]);
		if (!stack.empty() && d->is_liveout_regs(vnout->get_addr())) {
			vnnew = stack.back();
			vnnew->add_ref_point_simple(op);
		}
        stack.push_back(vnout);
        writelist.push_back(vnout);

		vnout->add_def_point_simple();
    }

    for (i = 0; i < bl->out.size(); ++i) {
        subbl = bl->get_out(i);
        slot = bl->get_out_rev_index(i);
        for (suboiter = subbl->ops.begin(); suboiter != subbl->ops.end(); suboiter++) {
            multiop = *suboiter;
            if (multiop->opcode != CPUI_MULTIEQUAL)
                break;

            vnin = multiop->get_in(slot);
            //if (vnin->is_heritage_known()) continue;
            if (vnin->flags.annotation) continue;

            vector<varnode *> &stack(varstack[vnin->get_addr()]);
            if (stack.empty()) {
                vnnew = new_varnode(vnin->size, vnin->get_addr());
                vnnew = set_input_varnode(vnnew);
                vnnew->version = vermap[vnin->get_addr()];
                stack.push_back(vnnew);
            }
            else
                vnnew = stack.back();

			vnnew->add_ref_point_simple(multiop);

            op_set_input(multiop, vnnew, slot);
            if (!vnin->uses.size()) {
                delete_varnode(vnin);
            }
        }
    }

    i = bl->index;
    for (slot = 0; slot < domchild[i].size(); ++slot)
        rename_recurse(domchild[i][slot], varstack, vermap);

    /*
    假如这个节点是出口节点，切变量为系统寄存器，则加入出口活跃变量集合
    */
    /*  对于noreturn 函数我们设置出口寄存器活跃数量为0 */
    if (bl->is_end() && !bl->noreturn()) {
        variable_stack::iterator it;
        for (it = varstack.begin(); it != varstack.end(); it++) {
            vector<varnode *> &stack = it->second;
            pair<pcodeop_def_set::iterator, bool> check;
            varnode *v;
            /*
            理论上所有的cpu寄存器都可以出口活跃的，不过为了减少代码量，
            1. r0-r15
            2. s0-s31
            */

			if (!stack.empty() && d->is_liveout_regs((v = stack.back())->get_addr())  && v->def) {
                topname.insert(v->def);
				v->add_ref_point_simple(bl->last_op());
			}
        }
    }

    for (i = 0; i < writelist.size(); ++i) {
        vnout = writelist[i];

        vector<varnode *> &stack(varstack[vnout->get_addr()]);
        varnode *v = stack.back();
        stack.pop_back();
    }
}

void		funcdata::build_liverange()
{
    variable_stack varstack;

    build_liverange_recurse(bblocks.get_block(0), varstack);
}

void        funcdata::build_liverange_recurse(blockbasic *bl, variable_stack &varstack)
{
    /* 当前block内，被def过得varnode集合 */
    vector<varnode *> writelist;
	list<pcodeop *>::iterator oiter;
	pcodeop *op;
    varnode *vnout, *vnin, *vnnew;
    int i, slot, order;

    for (oiter = bl->ops.begin(), order = 0; oiter != bl->ops.end(); oiter++) {
        op = *oiter ;
		//op->start.setOrder(order);

		for (slot = 0; slot < op->inrefs.size(); ++slot) {
			vnin = op->get_in(slot);

			if (vnin->flags.annotation || (vnin->is_constant() && vnin->get_addr().isConstant()))
				continue;

			/* 省略掉load的虚拟节点 */
			if ((op->opcode == CPUI_LOAD) && (slot == 2))
				continue;

			vector<varnode *> &stack(varstack[vnin->get_addr()]);
			if (stack.empty()) {
				vnin->clear_cover();
				vnin->add_ref_point(op);
				stack.push_back(vnin);
			}
			else {
				vnnew = stack.back();
				vnnew->add_ref_point(op);
			}
		}

        vnout = op->output;
        if (vnout == NULL) continue;
		if (op->opcode == CPUI_STORE) continue;

		vector<varnode *> &stack(varstack[vnout->get_addr()]);
		if (!stack.empty() && d->is_liveout_regs(vnout->get_addr())) {
			vnnew = stack.back();
			vnnew->add_ref_point(op);
		}

        stack.push_back(vnout);
        writelist.push_back(vnout);
		vnout->add_def_point();
    }

    i = bl->index;
    for (slot = 0; slot < domchild[i].size(); ++slot)
        build_liverange_recurse(domchild[i][slot], varstack);

    if (bl->is_end()) {
        variable_stack::iterator it;
        for (it = varstack.begin(); it != varstack.end(); it++) {
            vector<varnode *> &stack = it->second;
            pair<pcodeop_def_set::iterator, bool> check;
            varnode *v;
			if (!stack.empty() && d->is_liveout_regs((v = stack.back())->get_addr())) {
				v->add_ref_point(bl->last_op());
			}
        }
    }

    for (i = 0; i < writelist.size(); ++i) {
        vnout = writelist[i];

        vector<varnode *> &stack(varstack[vnout->get_addr()]);
        varnode *v = stack.back();
        stack.pop_back();
    }
}

/* calc_multiequal 
Algorithms for Computing the Static Single Assignment Form. P39 
*/
void        funcdata::calc_phi_placement(const vector<varnode *> &write)
{
    pq.reset(maxdepth);
    merge.clear();

    int i, j;
    flowblock *bl;

    for (i = 0; i < write.size(); ++i) {
        bl = write[i]->def->parent;
        j = bl->index;
        if (phiflags[j] & mark_node)
            continue;

        pq.insert(bl, domdepth[j]);
        phiflags[j] |= mark_node;
    }

    if ((phiflags[0] & mark_node) == 0) {
        pq.insert(bblocks.get_block(0), domdepth[0]);
        phiflags[0] |= mark_node;
    }

    while (!pq.empty()) {
        bl = pq.extract();
        visit_incr(bl, bl);
    }

    for (i = 0; i < phiflags.size(); ++i)
        phiflags[i] &= ~(mark_node | merged_node );
}

void        funcdata::calc_phi_placement2(const vector<varnode *> &write)
{
    vector<flowblock *>     blks;
    flowblock *bl;
    int i;

    for (i = 0; i < write.size(); i++) {
        bl = write[i]->def->parent;
        blks.push_back(bl);
    }

    calc_phi_placement3(blks);
}

void        funcdata::calc_phi_placement3(const vector<flowblock *> &write)
{
    int i;
    flowblock *bl;
    pq.reset(maxdepth);
    merge.clear();

    for (i = 0; i < write.size(); ++i) {
        bl = write[i];
        pq.insert(bl, domdepth[bl->index]);
    }

    while (!pq.empty()) {
        bl = pq.extract();
        phiflags[bl->index] |= visit_node;
        visit_dj(bl, bl);
    }

    for (i = 0; i < phiflags.size(); ++i)
        phiflags[i] = 0;
}

void        funcdata::visit_dj(flowblock *cur, flowblock *v)
{
    int i;

    for (i = 0; i < v->out.size(); i++) {
        flowblock *out = v->get_out(i);

        if (out->immed_dom == v) continue;

        if (domdepth[out->index] <= domdepth[cur->index]) {
            if ((phiflags[out->index] & merged_node) == 0) {
                merge.push_back(out);
                phiflags[out->index] |= merged_node;
            }

            if ((phiflags[out->index] & mark_node) == 0) {
                phiflags[out->index] |= mark_node;
                pq.insert(out, domdepth[out->index]);
            }
        }
    }

    for (i = 0; i < v->out.size(); i++) {
        flowblock *out = v->get_out(i);

        /* J-edge skip */
        if (out->immed_dom != v) continue;

        if ((phiflags[out->index] & visit_node) == 0) {
            phiflags[out->index] |= visit_node;
            visit_dj(cur, out);
        }
    }
}

flowblock*  funcdata::get_vmhead(void)
{
    int i, max_count = -1, t;
    flowblock *max = NULL;

    if (vmhead) 
        return vmhead->flags.f_dead ? NULL : vmhead;

    for (i = 0; i < bblocks.blist.size(); i++) {
        t = bblocks.blist[i]->get_back_edge_count();
        if (t  > max_count) {
            max_count = t;
            max = bblocks.blist[i];
        }
    }

    return vmhead = max;
}

flowblock*  funcdata::get_vmhead_unroll(void)
{
    flowblock *h = get_vmhead();

    if (!h) return NULL;

    flowblock *start = loop_pre_get(h, 0);

    while ((start->in.size() == 1) && (start->get_in(0)->out.size() == 1))
        start = start->get_in(0);

    return start;
}

pcodeop*    funcdata::get_vmcall(flowblock *b)
{
    pcodeop *p;
    list<pcodeop *>::iterator it;

    for (it = b->ops.begin(); it != b->ops.end(); it++) {
        p = *it;
        if (p->is_call() && d->test_cond_inline(d, p->get_call_offset()))
            return p;
    }

    return NULL;
}

flowblock*  funcdata::ollvm_get_head(void)
{
    int i;

    for (i = 0; i < ollvm.heads.size(); i++) {
        ollvmhead *oh = ollvm.heads[i];

        if (!oh->h->flags.f_dead) return oh->h;
    }

    return NULL;
}

int         funcdata::ollvm_detect_frameworkinfo()
{
    int i, t, j, k;
    ollvmhead *head;
    flowblock *b, *b1, *b2;

    for (i = 0; i < bblocks.blist.size(); i++) {
        t = bblocks.blist[i]->get_back_edge_count();
        if (t > 0) {
            head = new ollvmhead(bblocks.get_block(i));
            if (!ollvm_detect_fsm(head)) {
                ollvm.heads.push_back(head);
                head->h->mark_unsplice();
                printf("ollvm head[%llx] ", head->h->get_start().getOffset());
                if (!head->st1.isInvalid())
                    printf("fsm1[%s] ", d->trans->getRegisterName(head->st1.getSpace(), head->st1.getOffset(), head->st1_size).c_str());
                if (!head->st2.isInvalid())
                    printf("fsm2[%s] ", d->trans->getRegisterName(head->st1.getSpace(), head->st1.getOffset(), head->st1_size).c_str());
                printf("\n");
            }
            else
                delete head;
        }
    }

    vector<flowblock *> q;

    for (i = 0; i < bblocks.exitlist.size(); i++) {
        b = bblocks.exitlist[i];

        b->flags.f_exitpath = 1;
        q.push_back(b);

        while (!q.empty()) {
            b1 = q.front();
            q.erase(q.begin());

            for (j = 0; j < b1->in.size(); j++) {
                b2 = b1->get_in(j);
                if (!b2->flags.f_exitpath && !b2->loopheader && b2->loopnodes.empty()) {
                    b2->flags.f_exitpath = 1;
                    q.push_back(b2);
                }
            }
        }
    }

    printf("search maybe safezone alias start...\n");
    /* FIXME:后面要改成递归收集所有的load */
    vector<pcodeop *> poss;
    for (i = 0; i < ollvm.heads.size(); i++) {
        ollvmhead *head = ollvm.heads[i];
        pcodeop *p = head->h->find_pcode_def(head->st1), *p1, *p2;
        for (j = 0; j < head->h->in.size(); j++) {
            varnode *v = p->get_in(j);

            if (NULL == (p1 = v->def)) NULL;

            if ((p1->opcode == CPUI_LOAD)) 
                poss.push_back(p1->get_in(1)->def);
            else if (p1->opcode == CPUI_MULTIEQUAL) {
                for (k = 0; k < p1->inrefs.size(); k++) {
                    v = p1->get_in(k);
                    if (!(p2 = v->def)) continue;
                    if ((p2->opcode == CPUI_LOAD)) 
                        poss.push_back(p2->get_in(1)->def);
                }
            }
        }
    }
    for (i = 0; i < poss.size(); i++) {
        pcodeop *p = poss[i];
        char buf[128];
        p->dump(buf, PCODE_DUMP_SIMPLE & ~PCODE_HTML_COLOR);
        printf("%s\n", buf);
        if (p->output->is_rel_constant())
            set_safezone(p->output->get_val(), p->output->get_size());
    }
    printf("search maybe safezone alias end...\n");

    return 0;
}

int         funcdata::ollvm_detect_propchains(flowblock *&from, blockedge *&outedge)
{
    int i, ret;

    /* 寻找传递链的逻辑 
    1. 找到所有带循环的头 
    2. 确实每个带循环头的状态变量

    情况一: cmp rn, rm
        假如 rn 是常数，那么rm就是状态变量候选，否则rn是候选
        继续往上搜，假如
    */
    for (i = 0; i < ollvm.heads.size(); i++) {
        ollvmhead *oh = ollvm.heads[i];

        if (oh->h->flags.f_dead) continue;

        if (!ollvm_detect_propchain(oh, from, outedge, F_OPEN_PHI)) return 0;
    }

    /* 尝试传递拷贝复制 */
    for (i = 0; i < ollvm.heads.size(); i++) {
        ollvmhead *oh = ollvm.heads[i];

        if (oh->h->flags.f_dead) continue;

        ret = ollvm_detect_propchain(oh, from, outedge, F_OPEN_COPY);
        /* 执行了 lcs，在来一次copy */
        if (ret == 1)
            ret = ollvm_detect_propchain(oh, from, outedge, F_OPEN_COPY);

        /* 传递拷贝成功 */
        if (ret == 0) {
            /* 在搜索一次 */
            if (!ollvm_detect_propchain(oh, from, outedge, F_OPEN_PHI)) return 0;
        }
    }

    return -1;
}

int block_dfnum_cmp(blockedge *l, blockedge *r)
{
    return l->point->dfnum < r->point->dfnum;
}

int         funcdata::ollvm_detect_propchain(ollvmhead *oh, flowblock *&from, blockedge *&outedge, uint32_t flags)
{
    if (oh->h->flags.f_dead) return -1;

    flowblock *h = oh->h, *pre, *cur, *h1;
    int i, j, top;
    pcodeop *p = h->find_pcode_def(oh->st1), *p1;
    varnode *vn;
    blockedge *e;
    vector<blockedge *> invec;
    if (!p || (p->opcode != CPUI_MULTIEQUAL))
        return -1;

    for (i = 0; i < h->in.size(); i++) {
        e = & h->in[i];
        e->index = i;
        invec.push_back(&h->in[i]);
    }

    std::sort(invec.begin(), invec.end(), block_dfnum_cmp);

    p1 = p;
    for (i = 0; i < invec.size(); i++) {
        e = invec[i];
        pre = e->point;
        cur = h;
        vn = p->get_in(e->index);

        if (!vn->is_constant()) {
            p1 = vn->def;
            h1 = p1->parent;

            if (!p1->flags.mark_cond_copy_prop && ((p1->opcode == CPUI_COPY) || (p1->opcode == CPUI_LOAD)) && b_is_flag(flags,F_OPEN_COPY)) {
                vector<varnode *> defs;

                if ((p1->opcode == CPUI_LOAD) && !p1->have_virtualnode())
                    throw LowlevelError("alias analysis error");

                if (p1->opcode == CPUI_COPY) {
                    if (ollvm_combine_lcts(p1)) {
                        heritage_clear();
                        heritage();
                        dump_cfg(name, "lcts", 1);
                        return 1;
                    }
                }

                top = collect_all_const_defs(p1, defs);
                p1->flags.mark_cond_copy_prop = 1;

                if (defs.size() > 1) {
                    cond_copy_expand(p1, pre->get_out_index(cur));
                    heritage_clear();
                    heritage();
                    dump_cfg(name, "cond_copy_expand", 1);
                    return 0;
                }
            }
            else if ((p1->opcode == CPUI_MULTIEQUAL) && b_is_flag(flags, F_OPEN_PHI)) {
                for (j = 0; j < h1->in.size(); j++) {
                    vn = p1->get_in(j);
                    pre = p1->parent->get_in(j);
                    cur = p1->parent;

                    if (vn->is_constant()) break;
                }
            }
        }

        if (!vn->is_constant()) continue;

        from = pre;
        e = &pre->out[pre->get_out_index(cur)];

        /* 假如已经被标记过，不可计算了 */
        if (e->is(a_unpropchain)) continue;

        outedge = e;

        return 0;
    }

    return -1;
}

int         funcdata::ollvm_detect_fsm(ollvmhead *oh)
{
    flowblock *h = oh->h;
    list<pcodeop *>::reverse_iterator it;
    varnode *in0, *in1, *in;

    for (it = h->ops.rbegin(); it != h->ops.rend(); it++) {
        pcodeop *p = *it;

        /* 这个地方有点硬编码了，直接扫描sub指令，这个是因为当前的测试用例中的核心VM，用了cmp指令以后
        生成了sub，这个地方可能更好的方式是匹配更复杂的pattern */
        if (p->opcode == CPUI_INT_SUB) {
            in0 = p->get_in(0);
            in1 = p->get_in(1);

            if (!in0->is_constant() && !in1->is_constant())
                throw LowlevelError("ollvm_detect_fsm not support two state");

            in = in0->is_constant() ? in1 : in0;

            Address s = in->get_addr();

            for (++it; it != h->ops.rend(); it++) {
                p = *it;
                if ((p->opcode == CPUI_COPY) && p->output->get_addr() == s) {
                    oh->st1 = p->get_in(0)->get_addr();
                    oh->st1_size = p->get_in(0)->get_size();
                    return 0;
                }
            }

            oh->st1 = in0->is_constant() ? in1->get_addr() : in0->get_addr();
            oh->st1_size = in0->is_constant() ? in1->get_size() : in0->get_size();
            return 0;
        }
    }

    return -1;
}

bool        funcdata::use_outside(varnode *vn)
{
    return false;
}

/* 这里我调整了Ghidra的做法，原始的Ghidra为了更好的兼容性做了很多的考虑 */
void        funcdata::use2undef(varnode *vn)
{
    pcodeop *op;
    list<pcodeop *>::const_iterator iter;
    int i, size;
    bool res;

    res = false;
    size = vn->size;
    iter = vn->uses.begin();
    while (iter != vn->uses.end()) {
        op = *iter++;
        if (op->parent->is_dead()) continue;
        assert(op->parent->in.size());
        i = op->get_slot(vn);

        if (op->opcode != CPUI_MULTIEQUAL)
            throw LowlevelError("use2undef only support CPUI_MULTIEQUAL");

        op_remove_input(op, i);

        /* phi节点的in不能为0 */
        assert(op->inrefs.size());
    }
}

void        funcdata::branch_remove(blockbasic *bb, int num)
{
    branch_remove_internal(bb, num);
    structure_reset();
}

void        funcdata::branch_remove_internal(blockbasic *bb, int num)
{
    blockbasic *bbout;
    list<pcodeop *>::iterator iter;
    int blocknum;

	pcodeop *last = bb->last_op();
	if (last && (last->opcode == CPUI_CBRANCH) && (bb->out.size() == 2))
		op_destroy(last, 1);

    bbout = (blockbasic *)bb->get_out(num);
    blocknum = bbout->get_in_index(bb);
    bblocks.remove_edge(bb, bbout);
    clear_block_phi(bbout);
#if 0
    for (iter = bbout->ops.begin(); iter != bbout->ops.end(); iter++) {
        op = *iter;
        if (op->opcode != CPUI_MULTIEQUAL) continue;

        /* 当删除一个branch的分支节点时，他的分支里面的phi节点需要清理掉对应的in节点*/
        op_remove_input(op, blocknum);
        op_zero_multi(op);
    }
#endif
}

void        funcdata::block_remove_internal(blockbasic *bb, bool unreachable)
{
    list<pcodeop *>::iterator iter;
    pcodeop *op;

    op = bb->last_op();
    if (op) {
        assert(op->opcode != CPUI_BRANCHIND);
    }

    bblocks.remove_from_flow(bb);

    iter = bb->ops.begin();
    while (iter != bb->ops.end()) {
        op = *iter;
        iter++;
        op_destroy_ssa(op);
    }
    bblocks.remove_block(bb);
}

bool        funcdata::remove_unreachable_blocks(bool issuewarnning, bool checkexistence)
{
    vector<flowblock *> list;
    int i;

    bblocks.collect_reachable(list, bblocks.get_entry_point(), true);
    if (list.size() == 0) return false;

    for (i = 0; i < list.size(); i++) {
        list[i]->set_dead();
    }

    for (i = 0; i < list.size(); i++) {
        blockbasic *bb = list[i];
        while (bb->out.size() > 0)
            branch_remove_internal(bb, 0);
    }

    for (i = 0; i < list.size(); ++i) {
        blockbasic *bb = list[i];
        block_remove_internal(bb, true);
    }
    structure_reset();
    return true;
}

void        funcdata::splice_block_basic(blockbasic *bl)
{
    blockbasic *outbl = NULL;
    if (bl->out.size() == 1) {
        outbl = bl->get_out(0);
        if (outbl->in.size() != 1)
            outbl = NULL;
    }

    if (!outbl)
        throw LowlevelError("cannot splice basic blocks");

    if (!bl->ops.empty()) {
        pcodeop *jumpop = bl->last_op();
        if ((jumpop->opcode == CPUI_BRANCH) || (jumpop->opcode == CPUI_CBRANCH))
            op_destroy(jumpop, 1);
    }

    if (!outbl->ops.empty()) {
        pcodeop *firstop = outbl->ops.front();
        if (firstop->opcode == CPUI_MULTIEQUAL)
            throw LowlevelError("splicing block with MULTIEQUAL");
        firstop->flags.startblock = 0;
        list<pcodeop *>::iterator iter;

        for (iter = outbl->ops.begin(); iter != outbl->ops.end(); iter++) {
            pcodeop *op = *iter;
            op->parent = bl;
        }

        bl->ops.splice(bl->ops.end(), outbl->ops, outbl->ops.begin(), outbl->ops.end());
        /* Ghidra中有个set_order的操作，但是我们因为用不到order这个域，所以不用调这个函数 */
        // bl->set_order();
    }
    bblocks.splice_block(bl);
    structure_reset();
}

void        funcdata::remove_empty_block(blockbasic *bl)
{
    assert(bl->out.size() == 1);
    /* libmakeurl2.4.9.so 的 3648 出现了自己指向自己的情况，我们不允许删除这样的节点 */
    assert(bl->get_out(0) != bl);
    vector<flowblock *> inlist;
    int i, lab;

    flowblock *prev;
    flowblock *next = bl->get_out(0);

    for (i = 0; i < bl->in.size(); i++)
        inlist.push_back(bl->get_in(i));

    bblocks.remove_edge(bl, next);
    for (i = 0; i < inlist.size(); i++) {
        prev = inlist[i];
        lab = bblocks.remove_edge(prev, bl);
        bblocks.add_edge(prev, next, lab & a_true_edge);

        if ((prev->out.size() == 2) && (prev->get_out(0) == prev->get_out(1))) {
            clear_block_phi(prev->get_out(0));
            bblocks.remove_edge(prev, prev->get_out(0));

            if (prev->last_op()->opcode == CPUI_CBRANCH) {
                op_destroy(prev->last_op(), 1);
            }
        }
    }

    block_remove_internal(bl, true);

    /* 处理掉cbranch的block true , false指向同一个节点的情况 */

    structure_reset();
}

void        funcdata::redundbranch_apply()
{
    int i;
    flowblock *bb, *bl;

    for (i = 0; i < bblocks.get_size(); i++) {
        bb = bblocks.get_block(i);
        /* 

        1. 假如一个块已经空了
        2. 输出节点都为1
        3. 不是被vm标记过的节点 
        */
        if ((bb->out.size() == 1) && (bb->get_out(0) != bb) && bb->is_empty()) {
            /* 
            */
            if ((bb->vm_byteindex != -1) || (bb->vm_caseindex)) {
                remove_empty_block(bb);
                i -= 1;
            }
        }
    }

    for (i = 0; i < bblocks.get_size(); i++) {
        bb = bblocks.get_block(i);
        if (bb->out.size() == 0)
            continue;

        bl = bb->get_out(0);
        if (bb->out.size() == 1) {
            if ((bl->in.size() == 1) && !bl->is_entry_point() && !bb->is_switch_out() && !bl->is_unsplice()) {
                //printf("%sfound a block can splice, [%llx, %d]\n", print_indent(), bl->get_start().getOffset(), bl->dfnum);

                splice_block_basic(bb);
                i -= 1;
            }
        }
    }

	structure_reset();
}

void        funcdata::dump_store_info(const char *postfix)
{
    char obuf[256];
    list<pcodeop *>::const_iterator it;
    pcodeop *op;
    varnode *vn;
    FILE *fp;
	int i = 0;

    sprintf(obuf, "%s/%s/store_%s.txt", d->filename.c_str(), name.c_str(), postfix);

    fp = fopen(obuf, "w");

    for (it = storelist.begin(); it != storelist.end(); it++) {
        op = *it;
        if (op->is_dead()) {
            printf("op store[%d] is dead, %d\n", op->start.getTime(), ++i);
            continue;
        }
        vn = op->get_in(1);

        print_vartype(d->trans, obuf, vn);
        fprintf(fp, "(%s)\n", obuf);

        op->dump(obuf, PCODE_DUMP_SIMPLE & ~PCODE_HTML_COLOR);
        fprintf(fp, "%s\n", obuf);
    }

    fclose(fp);

	printf("oplist.size = %d\n", deadlist.size());
}

void        funcdata::dump_load_info(const char *postfix)
{
}

flowblock*  funcdata::split_block(flowblock *f, list<pcodeop *>::iterator it)
{
    flowblock *b = bblocks.new_block_basic();

    user_offset += user_step;
    Address addr(d->trans->getDefaultCodeSpace(), user_offset);

    it++;
    while (it != f->ops.end()) {
        pcodeop *p = *it;
        it++;

        f->remove_op(p);
        b->add_op(p);
    }

    b->set_initial_range(addr, addr);

    return b;
}

bool        funcdata::refinement(const Address &addr, int size, const vector<varnode *> &readvars, const vector<varnode *> &writevars, const vector<varnode *> &inputvars)
{
    if (size > 128)
        vm_error("refinement size too bigger %d\n", size);

    vector<int> refine(size + 1, 0);
    build_refinement(refine, addr, size, readvars);
    build_refinement(refine, addr, size, writevars);
    build_refinement(refine, addr, size, inputvars);
    int lastpos = 0;
    for (int curpos = 1; curpos < size; ++curpos) {
        if (refine[curpos]) {
            refine[lastpos] = curpos - lastpos;
            lastpos = curpos;
        }
    }

    if (lastpos == 0)
        return false;
    refine[lastpos] = size - lastpos;
    remove13refinement(refine);
    vector<varnode *> newvn;
    for (int i = 0; i < readvars.size(); i++)
        refine_read(readvars[i], addr, refine, newvn);

    return true;
}

void        funcdata::refine_read(varnode *vn, const Address &addr, const vector<int> &refine, vector<varnode *> &newvn)
{
}

void        funcdata::refine_write(varnode *vn, const Address &addr, const vector<int> &refine)
{
    vector<varnode *> newvn;
    split_by_refinement(vn, addr, refine, newvn);
    if (newvn.empty())
        return;

    varnode *replacevn = new_unique(vn->get_size());
    pcodeop *def = vn->def;
    op_set_output(def, replacevn);
    split_pieces(newvn, def, vn->get_addr(), vn->get_size(), replacevn);
    total_replace(vn, replacevn);
    delete_varnode(vn);
}
void        funcdata::split_pieces(const vector<varnode *> &vnlist, pcodeop *insertop, const Address &addr, int size, varnode *startvn)
{
    Address opaddress;
    uintb baseoff;
    bool isbigendian;
    blockbasic *bl;
    list<pcodeop *>::iterator insertiter;

    isbigendian = addr.isBigEndian();
    if (isbigendian)
        baseoff = addr.getOffset() + size;
    else
        baseoff = addr.getOffset();
    if (insertop == NULL) {
        bl = bblocks.get_entry_point();
        insertiter = bl->ops.begin();
        opaddress = get_addr();
    }
    else {
        bl = insertop->parent;
        insertiter = insertop->basiciter;
        ++insertiter;
        opaddress = insertop->get_addr();
    }

    for (int i = 0; i < vnlist.size(); i++) {
        varnode *vn = vnlist[i];
        pcodeop *p = newop(2, opaddress);
        op_set_opcode(p, CPUI_SUBPIECE);
        uintb diff;
        if (isbigendian)
            diff = baseoff - (vn->get_offset() + vn->get_size());
        else
            diff = vn->get_offset() - baseoff;
        op_set_input(p, startvn, 0);
        op_set_input(p, create_constant_vn(diff, 4), 1);
        op_set_output(p, vn);
        op_insert(p, bl, insertiter);
    }
}

void        funcdata::split_by_refinement(varnode *vn, const Address &addr, const vector<int> &refine, vector<varnode *> &split)
{
    Address curaddr = vn->get_addr();
    int sz = vn->get_size();
    int diff = curaddr.getOffset() - addr.getOffset();
    int cutsz = refine[diff];
    if (sz <= cutsz)
        return;
    while (sz > 0) {
        varnode *vn2 = new_varnode(cutsz, curaddr);
        split.push_back(vn2);
        curaddr = curaddr + cutsz;
        sz -= cutsz;
        diff = curaddr.getOffset() - addr.getOffset();
        cutsz = refine[diff];
        if (cutsz > sz)
            cutsz = sz;
    }
}

void        funcdata::build_refinement(vector<int> &refine, const Address &addr, int size, const vector<varnode *> &vnlist)
{
    for (int i = 0; i < vnlist.size(); i++) {
        Address curaddr = vnlist[i]->get_addr();
        int sz = vnlist[i]->get_size();
        int diff = (int)(curaddr.getOffset() - addr.getOffset());
        refine[diff] = 1;
        refine[diff + sz] = 1;
    }
}

void        funcdata::remove13refinement(vector<int> &refine)
{
}

