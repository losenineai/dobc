
#include "sleigh.hh"
#include "dobc.hh"
#include "thumb_gen.hh"
#include <assert.h>
#include "vm.h"

#define STACKBASE           0x4000000

void AssemblyRaw::dump(const Address &addr, const string &mnem, const string &body) {
    if (mnem1) {
        dobc::singleton()->add_inst_mnem(addr, mnem);
        return;
    }

    /* 
    1.假如有外部传入的buf
    1.1 假如有开启HTML，那么用html的格式，把内容格式化输出到buf上
    1.2 假如没有开启HTML，就用普通格式输出到BUF
    2. 假如有传入fp指针，则输出到fp上
    3. 假如都没有，输出到标注输出上(stdout)
    */
    if (buf) {
        if (enable_html) {
            int simd = dobc::singleton()->is_simd(addr);
            sprintf(buf, "<tr>"
                "<td><font color=\"" COLOR_ASM_STACK_DEPTH "\">%03x:</font></td>"
                "<td><font color=\"" COLOR_ASM_ADDR "\">0x%04x:</font></td>"
                "<td align=\"left\"><font color=\"" COLOR_ASM_INST_MNEM "\">%s </font></td>"
                "<td align=\"left\"><font color=\"" COLOR_ASM_INST_BODY "\">%s</font></td>"
                "<td align=\"left\"><font color=\"" COLOR_ASM_INST_BODY "\">%s</font></td></tr>",
                sp, (int)addr.getOffset(), mnem.c_str(), body.c_str(), simd?"simd":" ");
        }
        else
            sprintf(buf, "0x%04x: %s %s", (int)addr.getOffset(), mnem.c_str(), body.c_str());
    }
    else if (fp) {
        fprintf(fp, "0x%04x: %s %s\n", (int)addr.getOffset(), mnem.c_str(), body.c_str());
    }
    else
        fprintf(stdout, "0x%04x: %s %s\n", (int)addr.getOffset(), mnem.c_str(), body.c_str());
}

int  funcdata::loop_dfa_connect(uint32_t flags)
{
    int i, inslot, ret, end, nend;
    flowblock *cur, *prev, *br,  *last, *h = ollvm_get_head(), *from;
    list<pcodeop *>::const_iterator it;
    pcodeop *p, *op, *cmp_sub;
    varnode *iv = NULL;
    blockedge *chain = NULL;

    if (ollvm_detect_propchains2(from, chain)) {
        printf("ollvm not found propchain\n");
        return -1;
    }

    prev = from;
    cur = chain->point;

    trace_push(prev->last_op());

    do {
        print_info("\tprocess flowblock sub_%llx", cur->get_start().getOffset());

        it = cur->ops.begin();
        inslot = cur->get_in_index(prev);

        for (ret = 0; it != cur->ops.end(); it++) {
            p = *it;

            br = NULL;
            ret = static_trace(p, inslot, &br);

#if 0
            //if (flags & _DUMP_PCODE) 
            {
                char buf[256];
                p->dump(buf, PCODE_DUMP_SIMPLE & ~PCODE_HTML_COLOR);
                printf("%s\n", buf);
            }
#endif

#if 0
            if ((cur->out.size() == 2) && (p->opcode == CPUI_INT_SUB))
            {
                char buf[256];
                int len = 0;
                len += print_vartype (d->trans, buf, p->get_in(0));
                len += sprintf(buf + len, "  <>  ");
                len += print_vartype(d->trans, buf + len, p->get_in(1));
                printf("%s\n", buf);
            }
#endif

            trace_push(p);
        }

        int iv_in_normal_loop = false;

#if 1
        if ((ret == ERR_MEET_CALC_BRANCH) && (cmp_sub = cur->get_core_cmp())) {
            if (cmp_sub->parent == cur)
                iv_in_normal_loop = cur->is_iv_in_normal_loop(cmp_sub);
        }
#else
        if ((ret == ERR_MEET_CALC_BRANCH) && (cmp_sub = cur->get_cbranch_sub_from_cmp(op2))) {
            iv_in_normal_loop = cur->is_iv_in_normal_loop(cmp_sub);
        }
#endif

        if ((cur->out.size() > 1) && ((ret != ERR_MEET_CALC_BRANCH) || iv_in_normal_loop)) {
            printf("found undefined-bcond node[sub_%llx, index:%d, dfnum:%d]\n", cur->get_start().getOffset(), cur->index, cur->dfnum);

            for (end = trace.size() - 1; trace[end]->parent == cur; end--) {
            }

            /* 假如trace的最后一个节点，直接等于from，那么证明，一个分支节点都没走过去，直接禁止掉这个edge */
            if (trace[end]->parent == from) {
                printf("warn: only one block, forbidden this edge\n");
                chain->set_flag(a_unpropchain);

                static_trace_restore();

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
    } while (!cur->is_out_loop());

    if (end == trace.size() - 1)
        printf("found exit node[sub_%llx, index:%d, dfnum:%d]\n", cur->get_start().getOffset(), cur->index, cur->dfnum);

    /* back指向的节点
    1. 当走出循环时，back指向循环外节点
    2. 当碰到undefined cbranch, trace.back指向了第一个无法计算的循环节点 */
    last = (end == trace.size() - 1) ? cur:trace.back()->parent;
    cur = bblocks.new_block_basic();

    user_offset += user_step;
    Address addr(d->getDefaultCodeSpace(), user_offset);
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
            Address addr(d->getDefaultCodeSpace(), p->get_in(0)->get_val());
            callfd = d->find_func(addr);
        }

        /* 删除所有中间的 跳转指令和phi节点 */
        if (((p->opcode == CPUI_BRANCH) || (p->opcode == CPUI_CBRANCH) || (p->opcode == CPUI_INDIRECT) || (p->opcode == CPUI_MULTIEQUAL) || (p->opcode == CPUI_BRANCHIND)))
            continue;

        //Address addr2(d->getDefaultCodeSpace(), p->get_addr().getOffset());
        Address addr2(p->get_addr());
        const SeqNum sq(addr2, op_uniqid++);
        op = cloneop(p, sq);
        op_insert(op, cur, cur->ops.end());

        /* 假如trace以后，发现某个函数可以被计算，把他加入trace列表 */
        if (callfd && !op->callfd) {
            add_callspec(op, callfd);
        }
    }

    static_trace_restore();

    cur->set_initial_range(addr, addr);
    trace_clear();

    clear_block_phi(chain->point);

    int lab = bblocks.remove_edge(from, chain->point);
    bblocks.add_edge(from, cur, lab & a_true_edge);
    bblocks.add_edge(cur, last);

    do {
        cond_constant_propagation();
    } while (!cbrlist.empty());

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

int     funcdata::dead_phi_detect(pcodeop *p, vector<pcodeop *> &deadlist)
{
    pcodeop_set visit;
    pcodeop_set::iterator it;
    pcodeop *op, *p1;
    list<pcodeop *>::iterator useit;
    vector<pcodeop *> philist;

    deadlist.clear();

    visit.insert(p);
    philist.push_back(p);
    deadlist.push_back(p);
    while (!philist.empty()) {
        op = philist.front();
        philist.erase(philist.begin());

        /* 任意一个use不是phi，则这个phi是有意义的 */
        for (useit = op->output->uses.begin(); useit != op->output->uses.end(); useit++) {
            p1 = *useit;
            if (p1->opcode != CPUI_MULTIEQUAL)
                return 0;

            if (is_out_live(p1))
                return 0;

            it = visit.find(p1);
            if (it == visit.end()) {
                visit.insert(p1);
                philist.push_back(p1);
                deadlist.push_back(p1);
            }
        }
    }

    return 1;
}

void        funcdata::dead_phi_elimination()
{
    int i;
    list<pcodeop *>::iterator it, nextit;

    for (i = 0; i < bblocks.blist.size(); i++) {
        flowblock *b = bblocks.blist[i];

        for (it = b->ops.begin(); it != b->ops.end(); it = nextit) {
            pcodeop *p = *it;
            nextit = ++it;

            if (p->flags.copy_from_phi) continue;
            if ((p->opcode != CPUI_MULTIEQUAL)) break;

            vector<pcodeop *> deadphi;
            if (dead_phi_detect(p, deadphi)) {
                for (i = 0; i < deadphi.size(); i++) {
                    /* 这里必须擦除一下，不擦的话，会导致整个出口活跃的集合数据错误.
                    
                    NOTE: 不过这里会擦除出口活跃集合会不会导致BUG？*/
                    if (is_out_live(deadphi[i]))
                        topname.erase(deadphi[i]);
                    op_destroy(deadphi[i], 1);
                }
            }
        }
    }
}

void        funcdata::dead_code_elimination(vector<flowblock *> &blks, uint32_t flags)
{
    flowblock *b;
    list<pcodeop *>::iterator it;
    list<pcodeop *> worklist;
    vector<flowblock *> marks;
    pcodeop *op;
    int i;

    marks.clear();
    marks.resize(bblocks.get_size());

    if ((flags & F_REMOVE_DEAD_PHI))
        dead_phi_elimination();

    for (i = blks.size() - 1; i >= 0; i--) {
        b = blks[i];

        marks[b->dfnum] = b;

        for (it = b->ops.begin(); it != b->ops.end(); it++) {
            op = *it;
            if ((op->output && op->output->has_no_use()) || (op->opcode == CPUI_STORE)) {
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

        /*
        store等于variable的write，但是它的删除规则和普通的var不一样

        1. 不能因为没有use就删除，因为它可能被may load使用，而mayload是算不清的
        2. 可以被重复store删除，重复store包括2种情况
            情况1:
                *p = a;
                *p = b;
            上面的一条store可以被删除

            情况2:
                a = *p1;
                *p2 = a;
                a = *p2;
                *p1 = a;
            其实就是同一个值拷来拷去
        */
        if (op->opcode == CPUI_STORE) {

            if (op->output->has_no_use() && op->flags.store_on_dead_path) goto dce_label;

            vector<pcodeop *> deadstorelist;

            pcodeop *load = op->find_same_pos_load(deadstorelist);
            if (!load) continue;

            for (i = 0; i < deadstorelist.size(); i++) {
                deadstorelist[i]->flags.store_on_dead_path = 1;
            }

            //printf("dead store, load[%d, %d] \n", op->start.getTime(), load->start.getTime());

            while (op->output->has_use()) {
                pcodeop *use = op->output->uses.front();
                int inslot = use->get_slot(op->output);
                //op_remove_input(use, inslot);
                varnode *in = load->get_in(2);

                if (in->def)
                    op_set_input(use, in, inslot);
                else
                    op_set_input(use, clone_varnode(in), inslot);
            }
        }

        if (op->output->has_use()) 
            continue;

        /* 有些函数是有副作用的，它的def即使没有use也是不能删除的 */
        if (op->is_call()) continue;
        if (is_out_live(op)) continue;

dce_label:
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
    return (v0->loc.isConstant() && (v0->loc.getOffset() == (uintb)(d->trans->getDefaultCodeSpace())));
}

bool        funcdata::is_sp_constant(varnode *v)
{
    return v->is_sp_constant();
}

void        funcdata::place_multiequal(void)
{
    LocationMap::iterator iter;
    vector<varnode *> readvars;
    vector<varnode *> writevars;
    vector<varnode *> inputvars;
    pcodeop *multiop, *p;
    varnode *vnin;
    flowblock *bl;
    int max, i, j, flags, safevn;

    for (iter = disjoint.begin(); iter != disjoint.end(); ++iter) {
        Address addr = (*iter).first;
        int size = (*iter).second.size;

        readvars.clear();
        writevars.clear();
        inputvars.clear();

        /* 只有在安全区域内的stack变量，才会插入phi */
        safevn = 0;
        if ((addr.getSpace() == d->getStackBaseSpace())) {
            if (!in_safezone(addr.getOffset(), 4))
                continue;

            safevn = 1;
        }

        static int trace = 0;

        trace++;

        max = collect(addr, size, readvars, writevars, inputvars, flags);


        /* FIXME:Ghidra这里的判断和我差别很大，后期必须得重新调试，深入分析下Ghidra逻辑 */
        /* 这里的作用是当对同一个地址的，不同size的访问要如何处理
        
        比如: s0, d0
        或者: al, ax, eax, rax，

        后期我们要改成对simd类型指令，为8字节访问
        对普通寄存器，改成wordsize访问
        */
#define IS_READ_WRITE_NOT_SAME(f)          !BIT0(f)

        if (writevars.empty())
            continue;

        if (IS_READ_WRITE_NOT_SAME(flags)) {
            if (refinement(addr, max, readvars, writevars, inputvars)) {
                iter = disjoint.find(addr);
                size = (*iter).second.size;
                readvars.clear();
                writevars.clear();
                inputvars.clear();
                collect(addr, size, readvars, writevars, inputvars, flags);
            }
        }

        /* 我们在插入phi节点的时候，有些节点是不需要计算phi节点的 
        
        1. uniq的变量，而且没人读
        2. 没人读，也没人写
        3. 非cpu寄存器系列的变量，没有相对跳转，而且不是safevn
        */

        /* uniq变量，不出口活跃 */
        if (readvars.empty() && (addr.getSpace()->getType() == IPTR_INTERNAL))
            continue;

        if (readvars.empty() && writevars.empty() )
            continue;

        if (!d->is_cpu_reg(addr) && !BIT1(flags) && !safevn)
            continue;

        if (!BIT1(flags))
            calc_phi_placement2(writevars);
        else {
            /* 有相对跳转的指令，调整计算支配边界的方式 */
            calc_phi_placement4(writevars);
        }

        for (i = 0; i < merge.size(); ++i) {
            bl = merge[i];

            list<pcodeop *>::iterator it = bl->ops.begin(), nextit;
            varnode *vnout = NULL;

            for (multiop = NULL; it != bl->ops.end(); it = nextit) {
                p = *it++;
                nextit = it;


                if ((p->opcode == CPUI_MULTIEQUAL) && (p->inrefs.size() != bl->in.size())) {
                    op_destroy_ssa(p);
                    continue;
                }

                if ((p->opcode != CPUI_MULTIEQUAL) && !p->flags.copy_from_phi) break;

                if ((p->output->get_addr() == addr)) {
                    multiop = p;
                    break;
                }
            }

            /* 假如说某个PHI节点已经被转成了copy节点，则说明这个值已经被常量化，这个节点在下次heritage
            时已经不需要在插入phi 了。 
            FIXME:这个理解是否正确？*/
            if (p->flags.copy_from_phi) continue;
            //if ((it != bl->ops.end()) && p->flags.copy_from_phi) continue;

            if (!multiop) {
                multiop = newop(bl->in.size(), d->zero_addr);
                vnout = new_varnode_out(size, addr, multiop);
                op_set_opcode(multiop, CPUI_MULTIEQUAL);
                op_insert_begin(multiop, bl);
                j = 0;
            }
            else {
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

    vermap.clear();

    rename_recurse(bblocks.get_block(0), varstack, vermap);

    disjoint.clear();
}

void        funcdata::rename_recurse(flowblock *bl, variable_stack &varstack, version_map &vermap)
{
    /* 当前block内，被def过得varnode集合 */
    vector<varnode *> writelist;
    flowblock *subbl;
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
                    //vnnew = new_varnode(vnin->size, vnin->get_addr());
                    vnnew = clone_varnode(vnin);
                    vnnew = set_input_varnode(vnnew);

                    if (vnin->is_sp_vn())
                        vnnew->version = -1;
                    else
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

        if (vnout->is_sp_vn()) {
            if (is_safe_sp_vn(vnout))
                stack.push_back(vnout);
        }
        else
            stack.push_back(vnout);
        writelist.push_back(vnout);

		vnout->add_def_point_simple();
    }

    for (i = 0; i < bl->out.size(); ++i) {
        subbl = bl->get_out(i);
        slot = bl->get_out_rev_index(i);
        for (suboiter = subbl->ops.begin(); suboiter != subbl->ops.end(); suboiter++) {
            multiop = *suboiter;
            if (multiop->opcode != CPUI_MULTIEQUAL) {
                if (multiop->flags.copy_from_phi) continue;
                break;
            }

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
        /* stackbase的vn的stack，可能是空的 */
        if (!stack.size()) continue;
        varnode *v = stack.back();
        stack.pop_back();
    }
}

void		funcdata::build_liverange()
{
    variable_stack varstack;

    build_liverange_recurse(bblocks.get_block(0), varstack);
}

void        funcdata::build_liverange_recurse(flowblock *bl, variable_stack &varstack)
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

void        funcdata::calc_phi_placement4(const vector<varnode *> &writes)
{
    int i;
    pcodeop *op;
    merge.clear();

    for (i = 0; i < writes.size(); i++) {
        varnode *vn = writes[i];
        op = vn->def;
        if (op && op->parent->is_rel_branch())
            merge.push_back(op->parent->get_out(0));
    }
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

int vmhead_dfnum_cmp(ollvmhead *l, ollvmhead *r)
{
    return l->h->dfnum < r->h->dfnum;
}

int         funcdata::ollvm_detect_frameworkinfo()
{
    int i, t;
    ollvmhead *head;
    flowblock *b;
    vector<flowblock *> blks;
    pcodeop *p;

    bblocks.collect_no_cmp_cbranch_block(blks);

    printf("found [%d] no sub cbranch block search... \n", blks.size());
    for (i = 0; i < blks.size(); i++) {
        blks[i]->dump();
    }
    printf("no sub cbranch block search end\n");
    /* 
    libmakeurl.so:
    1. 0x15521

    liblazarus:
    1. 0x15f09
    */
    rewrite_no_sub_cbranch_blks(blks);

    /* 检测循环中，哪些循环可能有ollvm */
    for (i = 0; i < bblocks.blist.size(); i++) {
        b = bblocks.blist[i];
        t = b->get_back_edge_count();
        /* 
        */
        if (t > 0 && !b->is_rel_cbranch()) {
            head = new ollvmhead(bblocks.get_block(i));
            if (!ollvm_detect_fsm2(head)) {
                ollvm.heads.push_back(head);
                head->h->mark_unsplice();
            }
            else
                delete head;
        }
    }

    std::sort(ollvm.heads.begin(), ollvm.heads.end(), vmhead_dfnum_cmp);

    for (i = 0; i < ollvm.heads.size(); i++) {
        ollvmhead *head = ollvm.heads[i];
        printf("ollvm head[%llx, index:%d, dfnum:%d] ", head->h->get_start().getOffset(), head->h->index, head->h->dfnum);
        if (!head->st1.isInvalid())
            printf("fsm1[%s] ", d->trans->getRegisterName(head->st1.getSpace(), head->st1.getOffset(), head->st1_size).c_str());
        if (!head->st2.isInvalid())
            printf("fsm2[%s] ", d->trans->getRegisterName(head->st1.getSpace(), head->st1.getOffset(), head->st1_size).c_str());
        printf("\n");
    }

    printf("search maybe safezone alias start...\n");
    /* FIXME:后面要改成递归收集所有的load */

#if 1
    pcodeop_set poss_set, visit;
    for (i = 0; i < ollvm.heads.size(); i++) {
        ollvmhead *head = ollvm.heads[i];
        p = head->h->find_pcode_def(head->st1);
        ollvm_collect_safezone(p, visit, poss_set, 0);
    }

    pcodeop_set::iterator it;

    /* FIXME:这里我们需要校验安全区域之间是否overlap */
    for (it = poss_set.begin(); it != poss_set.end(); it++) {
        p = *it;
        char buf[128];
        p->dump(buf, PCODE_DUMP_SIMPLE & ~PCODE_HTML_COLOR);
        printf("%s\n", buf);

        if (p->opcode == CPUI_LOAD) {
            set_safezone(pi2(p)->get_addr().getOffset(), p->output->get_size());
        }
        else {
            set_safezone(p->output->get_addr().getOffset(), p->output->get_size());
        }
    }

    /* 假如有安全区域，需要全部关联起来，但是不需要rename */
    if (poss_set.size()) {
        heritage_clear();
        heritage();
    }
    printf("search maybe safezone alias end...\n");

#endif
    ollvm_copy_expand_all_vmhead();

    return 0;
}

int         funcdata::ollvm_detect_propchains2(flowblock *&from, blockedge *&outedge)
{
    ollvmhead *oh;
    int i, ret;

    /* 寻找传递链的逻辑 
    1. 找到所有带循环的头 
    2. 确实每个带循环头的状态变量

    情况一: cmp rn, rm
        假如 rn 是常数，那么rm就是状态变量候选，否则rn是候选
        继续往上搜，假如
    */
    for (i = 0; i < ollvm.heads.size(); i++) {
        oh = ollvm.heads[i];

        if (oh->h->flags.f_dead || oh->h->flags.f_no_cmp) continue;

        ret = ollvm_detect_propchain4(oh, from, outedge, 0);
        if (!ret)
            goto success_label;
    }

    /* 尝试传递拷贝复制 */
    for (i = 0; i < ollvm.heads.size(); i++) {
        oh = ollvm.heads[i];

        if (oh->h->flags.f_dead || oh->h->flags.f_no_cmp) continue;

        /* lcs 或者拷贝传递 */
        while ((ret = ollvm_detect_propchain4(oh, from, outedge, F_OPEN_COPY)) == ERR_NEED_REDETECT);

        if (ret == 0)
            goto success_label;
    }

    if (0 == ollvm_detect_propchain3(from, outedge))
        goto success_label;

    return -1;

success_label:
    oh->times++;

    printf("vmhead progchain[%llx, times:%d, in:%d] \n", oh->h->get_start().getOffset(), oh->times, oh->h->in.size());
    from->dump();
    outedge->point->dump();

    return 0;
}

int block_dfnum_cmp(blockedge *l, blockedge *r)
{
    return l->point->dfnum < r->point->dfnum;
}

void        flowblock::get_inlist_on_dfsort(vector<blockedge *> &invec)
{
    blockedge *e;

    for (int i = 0; i < in.size(); i++) {
        e = & in[i];
        e->index = i;
        invec.push_back(&in[i]);
    }

    std::sort(invec.begin(), invec.end(), block_dfnum_cmp);
}

bool            flowblock::is_direct_connect_to(flowblock *to)
{
    flowblock *b = this;

    while (b->out.size() == 1) {
        b = b->get_out(0);

        if (b == to)
            return true;
    }

    return false;
}

int         funcdata::ollvm_detect_propchain4(ollvmhead *oh, flowblock *&from, blockedge *&outedge, uint32_t flags)
{
    if (oh->h->flags.f_dead) return -1;

    flowblock *h = oh->h, *pre, *cur;
    int i;
    pcodeop *p = h->find_pcode_def(oh->st1), *p1, *p2;
    blockedge *e;
    vector<blockedge *> invec;
    /*
    当我们搜索一个混淆的循环头(vmhead)时，一般我们都假定:

    1. vmhead中有一个cmp指令，cmp指令中的其中一个是主状态寄存器
    2. 主状态reg在vmhead中都有一个对应的def指令，这个指令一般都是 phi

    但是有一些特殊情况下是没有def指令的。这个一般是最后的快要脱出vmhead的阶段。

    1. 获取vmhead中的cmp
    2. 直接取了第一个操作数寄存器（这里有问题，理论上应该取第一个状态寄存器，可能是第一个，也可能是第2个）
    3. 取这个操作数寄存器的def
    4. 判断这个def是否是phi
    5. 假如不是，搜索拷贝链，到phi为止
    6. 判断这个phi是否在混淆头内(必须不在，否则p不为空)
    7. 判断这个phi的block，和当前混淆头的关系
    8. 假如邻接，直接做条件展开
    9. 假如不邻接，判断phi的block的post-dom，是否和vmhead邻接，是的话，在post-dom上做展开


    2021年8月22日:
    1. 下面的if没有完全实现上文中的算法
    2. 我感觉 整个大的if块，可能和非if阶段的逻辑合并到一起，但是我还没完全理顺
    */
    if (!p) {
        if ((p = h->get_cbranch_sub_from_cmp(p2))) {
            p1 = p->get_in(0)->def;
            /* libmakeurl:sub_15521 */
            if (b_is_flag(flags, F_OPEN_COPY)) {
                if ((p1->parent == h)) {
                    if (p1->opcode == CPUI_COPY)
                        p1 = p1->get_in(0)->def;
                    else if (p1->opcode == CPUI_LOAD)
                        p1 = p1->get_in(2)->def;
                }

                if (h->is_in((pre = p1->parent)))
                    return ollvm_on_unconst_def(p1, pre, h);

                bblocks.calc_post_dominator();
                pre = pre->post_immed_dom;
                if (h->is_in(pre))
                    return ollvm_on_unconst_def(p1, pre, h);

                throw LowlevelError("not expected error");
            }
            else
                return ERR_NOT_DETECT_PROPCHAIN;
        }
        else {
            h->flags.f_no_cmp = 1;
            return ERR_NO_CMP_PCODE;
        }
    }
    /* FIXME:这个分支我忘记是个什么情况了 */
    else if (p->opcode != CPUI_MULTIEQUAL)
        return -1;

    /* 把所有的入节点按照dfnum进行排序 */
    h->get_inlist_on_dfsort(invec);

    /* 调试用 */
    static int trace = 0;
    trace++;

    pcodeop_set visit;

    if (ollvm_find_first_const_def(p, -1, from, outedge, visit)) {
        return 0;
    }

    if (!b_is_flag(flags, F_OPEN_COPY))
        return ERR_NOT_DETECT_PROPCHAIN;

    p1 = p;
    for (i = 0; i < invec.size(); i++) {
        e = invec[i];
        pre = e->point;
        cur = h;
        p1 = p->get_in(e->index)->def;

        /* 饶了一圈形成回环 */
        if ((p2 = p1->output->search_copy_chain(CPUI_MULTIEQUAL, h)) && (p2->parent == h))
            continue;

        int ret = ollvm_on_unconst_def(p1, pre, cur);
        if (ret == ERR_NEED_REDETECT)
            return ret;
    }

    return ERR_NOT_DETECT_PROPCHAIN;
}

int         funcdata::ollvm_on_unconst_def(pcodeop *p1, flowblock *pre, flowblock *cur)
{
    if (p1->flags.mark_cond_copy_prop) return  ERR_NOT_DETECT_PROPCHAIN;

    pcodeop *p2;
    vector<varnode *> defs, tdefs;
    int top, dfnum;

    if (p1->opcode == CPUI_COPY) {
        if (ollvm_combine_lcts(p1)) {
            printf("ollvm detect propchains do lcts\n");
            heritage_clear();
            heritage();
            dump_cfg(name, "lcts", 1);
            return ERR_NEED_REDETECT;
        }
    }

    /* 这里的规则有点问题，实际上算是跑各种corner case了 */
    if ((p1->opcode == CPUI_MULTIEQUAL) && p1->all_inrefs_is_top() && p1->parent->is_empty(1)) {
        remove_empty_block(p1->parent);
        structure_reset();
        heritage_clear();
        heritage();
        return ERR_NEED_REDETECT;
    }

    flowblock *bb = p1->parent;
    if ((bb->out.size() == 1) && cur->is_adjacent(bb)) {
        varnode *vn1 = p1->output;

        if (p1->opcode != CPUI_MULTIEQUAL) {
            p2 = p1->output->search_copy_chain(CPUI_MULTIEQUAL, bb);

            if (p2->opcode != CPUI_MULTIEQUAL)
                goto cond_copy_label;

            p1 = p2;
        }

        p1 = p1->get_in(0)->def;
        pre = p1->parent;
        cur = bb;
    }

cond_copy_label:
    top = collect_all_const_defs(p1, defs, tdefs, dfnum);
    p1->flags.mark_cond_copy_prop = 1;

    if (defs.size() > 1) {
        cond_copy_expand(p1, pre, pre->get_out_index(cur));
        printf("ollvm detect propchains do copy_expand\n");
        heritage_clear();
        heritage();
        dump_cfg(name, "cond_copy_expand", 1);
        return ERR_NEED_REDETECT;
    }

    return ERR_NOT_DETECT_PROPCHAIN;
}

void        funcdata::ollvm_collect_safezone(pcodeop *phi, pcodeop_set &visit, pcodeop_set &safeset, int depth)
{
    int i;
    pcodeop *p1;
    varnode *in;

    if (visit.find(phi) != visit.end()) return;
    visit.insert(phi);

    if (phi->opcode != CPUI_MULTIEQUAL) {
        p1 = phi->output->search_copy_chain(CPUI_MULTIEQUAL, NULL);

        visit.insert(p1);

        if (p1->opcode != CPUI_MULTIEQUAL) {
            if ((p1->opcode == CPUI_LOAD)) {
                safeset.insert(p1);
                return;
            }
            else
                throw LowlevelError("not support");
        }

        phi = p1;
    }

    for (i = 0; i < phi->inrefs.size(); i++) {
        in = phi->get_in(i);
        p1 = in->def;

        if (in->is_constant()) continue;

        ollvm_collect_safezone(p1, visit, safeset, ++depth);
    }
}

bool        funcdata::ollvm_find_first_const_def(pcodeop *p, int outslot, flowblock *&from, blockedge *&outedge, pcodeop_set &visit)
{
    pcodeop *op;
    vector<blockedge *> invec;
    flowblock *b = p->parent, *pre, *h1;
    varnode *vn;

    if (visit.find(p) != visit.end()) return false;
    visit.insert(p);

    //printf("find first pocde const def:%d\n", p->start.getTime());

    if (p->opcode != CPUI_MULTIEQUAL) {
        op = p->output->search_copy_chain(CPUI_MULTIEQUAL, p->parent);

        if (op->opcode != CPUI_MULTIEQUAL) {
            if (op->output && op->output->is_constant()) {
                from = op->parent;
                outedge = &from->out[outslot];
                return true;
            }
            else {
                return false;
            }
        }
        p = op;
    }

    b->get_inlist_on_dfsort(invec);

    for (int i = 0; i < invec.size(); i++) {
        blockedge *e = invec[i];
        vn = p->get_in(e->index);
        op = vn->def;
        h1 = op->parent;

        pre = e->point;

        /* 假如状态节点的某个phi节点输入就是常数，直接开始遍历 */
        if (vn->is_constant()) {
            from = pre;
            e = &pre->out[pre->get_out_index(b)];
            /* 假如已经被标记过，不可计算了 */
            if (e->is(a_unpropchain)) continue;
            outedge = e;
            return true;
        }

        if (b->is_adjacent(h1) || ((h1->out.size() == 1) && b->is_adjacent(h1->get_out(0)))) {
            if (ollvm_find_first_const_def(op, e->reverse_index, from, outedge, visit))
                return true;
        }
    }

    return false;
}

int         funcdata::ollvm_detect_propchain3(flowblock *&from, blockedge *&outedge)
{
    int i;
    pcodeop *sub, *op;

    for (i = 0; i < bblocks.blist.size(); i++) {
        flowblock *b = bblocks.get_block(i);
        if (b->in.size() != 2) continue;
        if (!b->is_cbranch()) continue;

        sub = b->get_cbranch_sub_from_cmp(op);
        if (!sub) continue;
        
        op = sub->get_in(0)->def;

        if (!op || (op->parent != b) || (op->opcode != CPUI_MULTIEQUAL) || !op->all_inrefs_is_constant() || !op->all_inrefs_is_adj()) continue;

        from = b->get_in(0);
        outedge = &from->out[from->get_out_index(b)];
        return 0;
    }

    return -1;
}

int         funcdata::ollvm_copy_expand_all_vmhead()
{
    int i;

    for (i = 0; i < ollvm.heads.size(); i++) {
        while (0 == ollvm_copy_expand_vmhead_phi(ollvm.heads[i]));
    }

    return 0;
}

int         funcdata::ollvm_copy_expand_vmhead_phi(ollvmhead *oh)
{
    flowblock *h = oh->h, *pre, *cur, *h1;
    int i;
    pcodeop *p = h->find_pcode_def(oh->st1), *p1;
    varnode *vn;
    blockedge *e;
    vector<blockedge *> invec;

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

        p1 = vn->def;
        h1 = p1->parent;

        if (p1->opcode != CPUI_MULTIEQUAL)
            continue;

        /* 假如某个in节点是phi节点
        1. 刚好邻接vmhead，那么尝试搜索它
        */
        if (h->is_adjacent(h1)) 
            if (!ollvm_do_copy_expand (p1, pre, pre->get_out_index(cur))) return 0;
    }

    return -1;
}

int         funcdata::ollvm_detect_fsm2(ollvmhead *oh)
{
    flowblock *h = oh->h;
    varnode *in0, *in1, *in;
    pcodeop *p, *p1, *op2;
    int dfnum;

    p = h->get_cbranch_sub_from_cmp(op2);

    if (!p) return -1;

    in0 = p->get_in(0);
    in1 = p->get_in(1);

    /* 2个数都不是常数，需要递归扫描sub指令的 常数定义 */
    if (!in0->is_constant() && !in1->is_constant()) {
        vector<varnode *> defs, tdefs;

        if (in0->def && (in0->def->opcode == CPUI_MULTIEQUAL)) {
            collect_all_const_defs(in0->def, defs, tdefs, dfnum);

            if (defs.size() > 1) {
                oh->st1 = in0->get_addr();
                oh->st1_size = in0->get_size();
                return 0;
            }
        }

        return -1;
    }

    /* 假如是普通循环，则退出 */
    if (h->is_iv_in_normal_loop(p)) return -1;

    in = in0->is_constant() ? in1 : in0;

    Address s = in->get_addr();

    p1 = in->search_copy_chain(CPUI_MULTIEQUAL, NULL);
    if (p1->opcode != CPUI_MULTIEQUAL) {
        if (p1->opcode != CPUI_LOAD) {
            throw LowlevelError("only support load");
        }

        //set_safezone(poa(p1).getOffset(), p1->output->get_size());
        set_safezone(pi2(p1)->get_addr().getOffset(), pi2(p1)->get_size());
        heritage_clear();
        heritage();

        p1 = p1->get_virtualnode()->def;
        if (p1->opcode != CPUI_MULTIEQUAL)
            throw LowlevelError("load not alias success");
    }

    if (ollvm_check_fsm(p1)) return -1;

    oh->st1 = p1->output->get_addr();
    oh->st1_size = p1->output->get_size();

    return 0;
}

int         funcdata::ollvm_check_fsm(pcodeop *p)
{
    if (!p || (p->opcode != CPUI_MULTIEQUAL)) return -1;

    vector<intb>    consts;

    for (int i = 0; i < p->inrefs.size(); i++) {
        varnode *vn1 = p->get_in(i);
        if (vn1->is_constant())
            consts.push_back(vn1->get_val());
    }

    /* 只包含一个常数 */
    if (consts.size() == 1) {
        /* 我们认为包含的数字过小，不是被ollvm混淆过的 */
        if (consts[0] < 10) return -1;
    }

    return 0;
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

void        funcdata::branch_remove(flowblock *bb, int num)
{
    branch_remove_internal(bb, num);
    structure_reset();
}

void        funcdata::branch_remove_internal(flowblock *bb, int num)
{
    flowblock *bbout;
    list<pcodeop *>::iterator iter;
    int blocknum;

	pcodeop *last = bb->last_op();
	if (last && (last->opcode == CPUI_CBRANCH) && (bb->out.size() == 2))
		op_destroy(last, 1);

    bbout = (flowblock *)bb->get_out(num);
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

void        funcdata::block_remove_internal(flowblock *bb, bool unreachable)
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
        flowblock *bb = list[i];
        while (bb->out.size() > 0)
            branch_remove_internal(bb, 0);
    }

    for (i = 0; i < list.size(); ++i) {
        flowblock *bb = list[i];
        block_remove_internal(bb, true);
    }
    structure_reset();
    return true;
}

void        funcdata::splice_block_basic(flowblock *bl)
{
    flowblock *outbl = NULL;
    if (bl->out.size() == 1) {
        outbl = bl->get_out(0);
        if (outbl->in.size() != 1)
            outbl = NULL;
    }

    if (!outbl)
        throw LowlevelError("cannot splice basic blocks");

    if (!bl->ops.empty()) {
        pcodeop *jumpop = bl->last_op();
        if ((jumpop->opcode == CPUI_BRANCH) || (jumpop->opcode == CPUI_CBRANCH) || (jumpop->opcode == CPUI_BRANCHIND))
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

void        funcdata::remove_empty_block(flowblock *bl)
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
        if (bb->is_empty_delete() || bb->is_11_branch()) {
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
            //printf("op store[%d] is dead, %d\n", op->start.getTime(), ++i);
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

void        funcdata::dump_alias_info(FILE *fp)
{
    list<pcodeop *>::iterator it;

    struct {
        struct {
            set<Address> sp_set;
            int     sp_count = 0;
            int     argument_count = 0;
            int     top_count = 0;
            int     text_count = 0;
            int     bss_count = 0;
            list<pcodeop *> toplist;
        } load;

        struct {
            set<Address> sp_set;
            int     sp_count = 0;
            int     argument_count = 0;
            int     top_count = 0;
            int     text_count = 0;
            int     bss_count = 0;
            list<pcodeop *> toplist;
        } store;
    } stat;

    for (it = loadlist.begin(); it != loadlist.end(); it++) {
        pcodeop *p = *it;

        varnode *in1 = p->get_in(1);

        if (p->get_in(1)->is_constant())
            stat.load.text_count++;
        else if (p->get_in(1)->is_sp_constant()) {
            stat.load.sp_set.insert(p->get_in(2)->get_addr());
            stat.load.sp_count++;
        }
        else {
            stat.load.top_count++;
            stat.load.toplist.push_back(p);
        }
    }

    for (it = storelist.begin(); it != storelist.end(); it++) {
        pcodeop *p = *it;

        varnode *in1 = p->get_in(1);

        if (p->get_in(1)->is_constant())
            stat.store.text_count++;
        else if (p->get_in(1)->is_sp_constant()) {
            stat.store.sp_set.insert(p->output->get_addr());
            stat.store.sp_count++;
        }
        else {
            stat.store.top_count++;
            stat.store.toplist.push_back(p);
        }
    }

    fprintf(fp, "stat info =====================================================================\n");
    fprintf(fp, "load alias info:\n");
    fprintf(fp, "   sp_set_count:%d \n", stat.load.sp_set.size());
    fprintf(fp, "   sp_count:%d     \n", stat.load.sp_count);
    fprintf(fp, "   text_count:%d   \n", stat.load.text_count);
    fprintf(fp, "   top_count:%d    \n", stat.load.toplist.size());

    fprintf(fp, "store alias info:\n");
    fprintf(fp, "   sp_set_count:%d \n", stat.store.sp_set.size());
    fprintf(fp, "   sp_count:%d     \n", stat.store.sp_count);
    fprintf(fp, "   text_count:%d   \n", stat.store.text_count);
    fprintf(fp, "   top_count:%d    \n", stat.store.toplist.size());

    fprintf(fp, "call info:\n");
    fprintf(fp, "   count: %d\n", calllist.size());
    fprintf(fp, "stat info end********************************************************************\n\n");
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
    for (int i = 0; i < readvars.size(); i++)
        refine_read(readvars[i], addr, refine);
    for (int i = 0; i < writevars.size(); i++)
        refine_write(writevars[i], addr, refine);
    for (int i = 0; i < inputvars.size(); i++)
        refine_input(inputvars[i], addr, refine);

    LocationMap::iterator iter = disjoint.find(addr);
    int pass = (*iter).second.pass;
    disjoint.erase(iter);
    iter = globaldisjoint.find(addr);
    globaldisjoint.erase(iter);
    Address curaddr = addr;
    int cut = 0;
    int intersect;

    while (cut < size) {
        int sz = refine[cut];
        disjoint.add(curaddr, sz, pass, intersect);
        globaldisjoint.add(curaddr, sz, pass, intersect);
        cut += sz;
        curaddr = curaddr + sz;
    }

    return true;
}

varnode*        funcdata::concat_pieces(const vector<varnode *> &vnlist, pcodeop *insertop, varnode *finalvn)
{
    varnode *preexist = vnlist[0];
    bool isbigendian = preexist->get_addr().isBigEndian();
    Address opaddress;
    flowblock *bl;
    list<pcodeop *>::iterator   insertiter;

    if (insertop == NULL) {
        bl = bblocks.get_entry_point();
        insertiter = bl->ops.begin();
        opaddress = get_addr();
    }
    else {
        bl = insertop->parent;
        insertiter = insertop->basiciter;
        opaddress = insertop->get_addr();
    }

    for (int i = 1; i < vnlist.size(); i++) {
        varnode *vn = vnlist[i];
        pcodeop *op = newop(2, opaddress);
        op_set_opcode(op, CPUI_PIECE);
        varnode *newvn;
        if (i == vnlist.size() - 1) {
            newvn = finalvn;
            op_set_output(op, newvn);
        }
        else
            newvn = new_unique_out(preexist->get_size() + vn->get_size(), op);
        if (isbigendian) {
            op_set_input(op, preexist, 0);
            op_set_input(op, vn, 1);
        }
        else {
            op_set_input(op, vn, 0);
            op_set_input(op, preexist, 1);

        }
        op_insert(op, bl, insertiter);
        preexist = newvn;
    }

    return preexist;
}

void        funcdata::refine_read(varnode *vn, const Address &addr, const vector<int> &refine)
{
    vector<varnode *> newvn;
    split_by_refinement(vn, addr, refine, newvn);
    if (newvn.empty())
        return;

    varnode *replacevn = new_unique(vn->get_size());;
    pcodeop *op = vn->lone_use();
    int slot = op->get_slot(vn);
    concat_pieces(newvn, op, replacevn);
    op_set_input(op, replacevn, slot);
    if (vn->has_no_use())
        delete_varnode(vn);
    else
        throw LowlevelError("refining non-free varnode");
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

void        funcdata::refine_input(varnode *vn, const Address &addr, const vector<int> &refine)
{
    assert(0);
}

void        funcdata::split_pieces(const vector<varnode *> &vnlist, pcodeop *insertop, const Address &addr, int size, varnode *startvn)
{
    Address opaddress;
    uintb baseoff;
    bool isbigendian;
    flowblock *bl;
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

int funcdata::collect(Address addr, int size, vector<varnode *> &read, vector<varnode *> &write, vector<varnode *> &input, int &flags)
{
    varnode *vn;
    varnode_loc_set::const_iterator     viter = begin_loc(addr);
    varnode_loc_set::const_iterator     enditer;
    uintb start = addr.getOffset();
    pcodeop *op;
    addr = addr + size;

    if (addr.getOffset() < start) {
        //assert(0);
        Address tmp(addr.getSpace(), addr.getSpace()->getHighest());
        enditer = end_loc(tmp);
    }
    else
        enditer = begin_loc(addr);

    int maxsize = 0, prev = 0;

    flags = 0;
    BIT0_SET(flags);
    for (; viter != enditer; viter++) {
        vn = *viter;

        if (vn->flags.written) {
            if (vn->size > maxsize)     maxsize = vn->size;
            op = vn->def;
#if 1
            if (op && op->parent->is_rel_branch()) 
                BIT1_SET(flags);
#endif
            write.push_back(vn);
        }
        else if (!vn->is_heritage_known() && vn->uses.size())
            read.push_back(vn);
        else if (vn->flags.input)
            input.push_back(vn);

        if (prev && prev != vn->size) 
            BIT0_CLR(flags);
        prev = vn->size;
    }

    return maxsize;
}

void        funcdata::heritage(void)
{
    varnode_loc_set::const_iterator iter, enditer;
    varnode *vn;
    int i;

    heritage_times++;

    if (maxdepth == -1)
        build_adt();

    long start = clock();
    for (i = 0; i < d->numSpaces(); i++) {
        AddrSpace *spc = d->getSpace(i);

        if (!spc->isHeritaged())
            continue;

        iter = begin_loc(spc);
        enditer = end_loc(spc);

        while (iter != enditer) {
            vn = *iter++;

            if (!vn->flags.written && vn->has_no_use() && !vn->flags.input)
                continue;

#if 0
            char buf[128];
            print_varnode(d->trans, buf, vn);
            if (vn->def)
                printf("%s - %d\n", buf, vn->def->start.getTime());
            else
                printf("%s\n", buf);
#endif

            if (vn->is_sp_vn()) {
                if (!is_safe_sp_vn(vn))
                    continue;
            }

            int prev = 0;
            LocationMap::iterator iter = globaldisjoint.add(vn->get_addr(), vn->size, pass, prev);
            if (prev == 0)
                disjoint.add((*iter).first, (*iter).second.size, pass, prev);
            else {
                assert(0);
            }
        }
    }
    place_multiequal();
    rename();
	//build_liverange();

    clock_t start1 = clock();

    constant_propagation4();

    clock_t start2 = clock();

    generate_sp_info();


    print_info("%sheritage scan node end.  heriage[%lu]ms, CP[%lu]ms, sp[%lu]ms.", print_indent(), start1 - start, start2 - start1, clock() - start2);
}

void    funcdata::heritage_clear()
{
    topname.clear();
    disjoint.clear();
    globaldisjoint.clear();
    domchild.clear();
    augment.clear();
    phiflags.clear();
    domdepth.clear();
    merge.clear();
    //alias_clear();
    propchains.clear();

    maxdepth = -1;
    pass = 0;
}

int         funcdata::constant_propagation4()
{
    list<pcodeop *>::const_iterator iter;
    list<pcodeop *>::const_iterator iter1;
	vector<pcodeop *> topstorelist;
    pcodeop_set::iterator it;
    pcodeop_set set;
    pcodeop_set visit;
	pcodeop_set maystore_set;
    pcodeop *op, *use, *load, *store, *maystore;
	int ret = 0, r, changed = 0;
    flowblock *b;
    varnode *out;

    flags.enable_to_const = flags.enable_peephole;

    bblocks.collect_sideeffect_ops();

    for (iter = deadlist.begin(); iter != deadlist.end(); iter++) {
        op = *iter;
        if (op->flags.dead) continue;
        set.insert(op);
    }

cp_label1:
    while (!set.empty()) {
        it = set.begin();
        op = *it;
        set.erase(it);

        if (op->flags.dead) continue;

        /* 每个store，可能有很多mayuse节点挂在上面，当某个store地址算出以后，清空它身上的mayuse节点 */
        if ((op->opcode == CPUI_STORE) && !op->get_in(1)->is_top()) {
            for (iter1 = op->mayuses.begin(); iter1 != op->mayuses.end(); ++iter1) {
                use = *iter1;
                set.insert(use);
            }

            op->mayuses.clear();
        }

        if ((op->opcode == CPUI_LOAD) || (op->opcode == CPUI_STORE)) op->create_stack_virtual_vn();

        r = op->compute(-1, &b);

        if (flags.enable_to_const)
            op->to_constant1();

        out = op->output;

        if (!out) continue;

        int find = (visit.find(op) != visit.end());

        if (out->is_constant() || out->is_sp_constant()) {
            if (find) continue;
            visit.insert(op);

            for (iter1 = out->uses.begin(); iter1 != out->uses.end(); ++iter1) {
                set.insert(*iter1);
            }
        }
        else if ((op->opcode == CPUI_LOAD) &&  !op->flags.input && !find) {
            if (op->have_virtualnode() && is_safe_sp_vn(op->get_virtualnode()))
                continue;

            load = op;
            maystore = NULL;
            store = store_query(load, NULL, load->get_in(1), &maystore);

            if (!store) {
                if (maystore) {
                    maystore->add_mayuse(load);
					maystore_set.insert(maystore);
                }
                continue;
            }

            /* 某些load的值，并不是来源于store，而是来自于sp的分配
            
            在堆栈自底向上增长时，sp的减小，意味着stack alloc memory的行为

            sp = sp - 0x100

            我们这里假设这个alloc出来的值都为0
            */
            if (store->opcode != CPUI_STORE) {
                load->output->set_val(0);
                load->flags.val_from_sp_alloc = 1;
                continue;
            }

            set.insert(load);
            visit.insert(load);

            /* 这个调用子函数时，进入的流程，需要判断store的fd，是否等于当前fd*/
            if (store->parent->fd != this) {
                load->output->type = store->get_in(2)->type;
                load->flags.input = 1;
                continue;
            }

            varnode *in = store->get_in(1);

            /* 假如这个store已经被分析过，直接把store的版本设置过来 */
            if (!store->output)
                throw LowlevelError("store output empty");

            op_set_input(load, store->output, 2);
        }
    }

	topstorelist.clear();
	while (!maystore_set.empty()) {
        it = maystore_set.begin();
        op = *it;
        maystore_set.erase(it);

		if (op->flags.dead) continue;

		op->mayuses.clear();
		if (op->get_in(1)->type.height == a_top)
			topstorelist.push_back(op);
	}

	if (flags.enable_topstore_mark && !topstorelist.empty()) {
		sort(topstorelist.begin(), topstorelist.end(), pcodeop_domdepth_cmp());

		op = topstorelist[0];
		flowblock *b = op->parent;
		while (b) {
			if (b->in.size() > 1) break;

			b = b->in.size() ? b->get_in(0):NULL;
		}

		if (!b) {
			changed = 1;
			op->flags.uncalculated_store = 1;
			for (iter1 = op->mayuses.begin(); iter1 != op->mayuses.end(); iter1++) {
				set.insert(*iter1);
			}

			op->mayuses.clear();

			goto cp_label1;
		}
	}

    return ret;
}


int         funcdata::cond_constant_propagation()
{
    flowblock *parent, *to;
    blockedge *del_edge;
    pcodeop *op;
    varnode *in;
    int i;

    for (i = 0; i < cbrlist.size(); i++) {
        op = cbrlist[i];
        parent = op->parent;

        if (op->is_dead()) continue;

        //printf("%sfind cbr block:%llx:%d\n", print_indent(), op->get_addr().getOffset(), parent->dfnum);

        in = op->get_in(1);
        assert(in->is_constant());

        /* 获取要删除的边，所以要取相反条件的边 */
        del_edge = in->get_val() ? parent->get_false_edge() : parent->get_true_edge();
        to = del_edge->point;

        /* 删除cbranch上的条件判断，已经不需要了, 前面定义这个条件的语句也会在后面的死代码删除中去掉*/
        branch_remove_internal(parent, parent->get_out_index(to));

        /* 清楚标记的时候，我们并不在乎删除的是哪条边，反正把a_true_edge的标记清除就可以了 */
        parent->clear_out_edge_flag(0, a_true_edge);
    }

    cbrlist.clear();
    remove_unreachable_blocks(true, true);

    //printf("%safter cbr remove, now blocks size = %d, dead is = %d\n", print_indent(), bblocks.get_size(), bblocks.deadlist.size());

    redundbranch_apply();
    emptylist.clear();

    heritage_clear();
    heritage();

    return 0;
}

void    funcdata::compute_sp(void)
{
    if (!flags.blocks_generated)
        throw LowlevelError("compute sp need block generated");
}

void        funcdata::set_safezone(intb addr, int size)
{
    rangenode *range = new rangenode();

    /* 现在的safezone只能设置stack space上的 */
    range->start = STACKBASE + addr;
    range->size = size;

    safezone.push_back(range);
}

bool        funcdata::in_safezone(intb a, int size)
{
    rangenode *n;
    list<rangenode *>::iterator  it;

    a += STACKBASE;

    for (it = safezone.begin(); it != safezone.end(); it++) {
        n = *it;
        if ((a >= n->start) && (a + size) <= n->end())
            return true;
    }

    return false;
}

void        funcdata::enable_safezone(void)
{
    flags.safezone = 1;
}

void        funcdata::disable_safezone(void)
{
    flags.safezone = 0;
}

intb        funcdata::get_stack_value(intb offset, int size)
{
    u1 *p = memstack.bottom + offset;

    if ((offset > 0) || (offset + size - 1) > 0)
        throw LowlevelError(name + " memstack downflow");

    if (size & (size - 1))
        throw LowlevelError("get_stack_value not support size:" + size);

    if (size == 8)
        return *(intb *)p;
    else if (size == 4)
        return *(int *)p;
    else if (size == 2)
        return *(short *)p;
    else
        return p[0];
}

void        funcdata::set_stack_value(intb offset, int size, intb val)
{
    u1 *p = memstack.bottom + offset;

    if ((offset > 0) || (offset + size - 1) > 0)
        throw LowlevelError(name + " memstack downflow");

    memcpy(p, (char *)&val, size);
}

void        funcdata::add_to_codelist(pcodeop *op)
{
    switch (op->opcode) {
    case CPUI_STORE:
        op->codeiter = storelist.insert(storelist.end(), op);
        break;

    case CPUI_LOAD:
        op->codeiter = loadlist.insert(loadlist.end(), op);
        break;

    case CPUI_CALL:
    case CPUI_CALLIND:
        op->codeiter = calllist.insert(calllist.end(), op);
        break;

    default:break;
    }
}

void        funcdata::remove_from_codelist(pcodeop *op)
{
    switch (op->opcode) {
    case CPUI_LOAD:
        loadlist.erase(op->codeiter);
        break;

    case CPUI_STORE:
        storelist.erase(op->codeiter);
		break;

    case CPUI_CALLIND:
    case CPUI_CALL:
        calllist.erase(op->codeiter);
        break;
    }
}

bool        funcdata::test_hard_inline_restrictions(funcdata *inlinefd, pcodeop *op, Address &retaddr)
{
    list<pcodeop *>::iterator iter = op->insertiter;
    ++iter;

    /* 理论上即使指向end也应该可以inline的，但是这里为了处理方便，不关心末尾的特殊情况 */
    if (iter == deadlist.end())
        throw LowlevelError("No fallthrough prevents inline here");
 
    pcodeop *nextop = *iter;
    retaddr = nextop->get_addr();

    nextop->flags.startblock = 1;

    return true;
}

bool        funcdata::is_first_op(pcodeop *op)
{
    if (!flags.blocks_generated)
        return op == (*deadlist.begin());

    flowblock *b = bblocks.get_block(0);
    list<pcodeop *>::iterator it = b->ops.begin();

    return *it == op;
}

flowblock*    funcdata::loop_pre_get(flowblock *h, int index)
{
    flowblock *pre = NULL;
    int i, sizein = h->in.size();

    /* 查找入口节点的非循环前驱节点，也就是哪个节点可以进来 */
    for (i = 0; i < sizein; i++) {
        pre = h->get_in(i);
        
        if (pre->dfnum < h->dfnum)
            return pre;
    }

    return NULL;
}

void        funcdata::dump_phi_placement(int bid, int pid)
{
    flowblock *b = bblocks.get_block_by_index(bid);
    pcodeop *p = b->get_pcode(pid);

    if (!p) return;

    vector<varnode *> writevars;

    writevars.push_back(p->output);

    calc_phi_placement(writevars);
    printf("p%d merge point:", p->start.getTime());
    for (int i = 0; i < merge.size(); i++) {
        printf("%d ", merge[i]->index);
    }
    printf("\n");
}

bool        funcdata::is_out_live(pcodeop *op)
{
    pcodeop_def_set::iterator it = topname.find(op);
    if ((it != topname.end()))
        return true;

    return false;
}

varnode*    funcdata::detect_induct_variable(flowblock *h, flowblock * &exit)
{
    list<pcodeop *>::const_reverse_iterator it;
    flowblock *true1;

    if (NULL == (exit = bblocks.detect_whiledo_exit(h))) 
        throw LowlevelError("h:%d whiledo loop not found exit node");

    true1 = h->get_true_edge()->point;

    for (it = h->ops.rbegin(); it != h->ops.rend(); it++) {
        pcodeop *p = *it;

        /* 这个地方有点硬编码了，直接扫描sub指令，这个是因为当前的测试用例中的核心VM，用了cmp指令以后
        生成了sub，这个地方可能更好的方式是匹配更复杂的pattern */
        if (p->opcode == CPUI_INT_SUB) {
            varnode *in0 = p->get_in(0);
            varnode *in1 = p->get_in(1);

            /* 假如前面是cmp指令，而且节点节点等于真值出口，那么我们认为in1是归纳变量 */
            return  (exit == true1) ? in1:in0;
        }
    }

    throw LowlevelError("loop header need CPUI_INT_SUB");
}

bool        funcdata::can_analysis(flowblock *b)
{
    return true;
}

flowblock*        funcdata::combine_multi_in_before_loop(vector<flowblock *> ins, flowblock *header)
{
    int i;
    flowblock *b = bblocks.new_block_basic(user_offset += user_step);

    for (i = 0; i < ins.size(); i++) {
        int lab = bblocks.remove_edge(ins[i], header);
        bblocks.add_edge(ins[i], b, lab & a_true_edge);
    }

    bblocks.add_edge(b, header);

    clear_block_phi(header);

    structure_reset();

    return b;
}

void        funcdata::dump_exe()
{
    thumb_gen gen(this);

    gen.run();

    if (d->debug.dump_inst1)
        gen.dump();
}

void        funcdata::detect_calced_loops(vector<flowblock *> &loops)
{
    int i;
    flowblock *lheader;

    for (i = 0; i < bblocks.loopheaders.size(); i++) {
        lheader = bblocks.loopheaders[i];

        /* FIXME:硬编码了一波， */
        if ((lheader->loopnodes.size() == 2) && (lheader->dfnum < 10)) 
            loops.push_back(lheader);
    }
}

void        funcdata::remove_loop_livein_varnode(flowblock *lheader)
{
    int i;
    flowblock *o;
    list<pcodeop *>::iterator it, next;
    list<pcodeop *>::iterator itu;
    pcodeop *p, *p_use;
    varnode *out;

    /* 遍历循环内所有节点 */
    for (i = 0; i < lheader->loopnodes.size(); i++) {
        o = lheader->loopnodes[i];

        for (it = o->ops.begin(); it != o->ops.end(); it = next) {
            p = *it;
			next = ++it;

            if (!(out = p->output)) continue;

            /* 查看循环内的varnode 是否有被循环歪的节点使用到，假如是的话，则把这个节点加入到outlist中去*/
            for (itu = out->uses.begin(); itu != out->uses.end(); itu++) {
                p_use = *itu;
                if (!bblocks.in_loop(lheader, p_use->parent)) break;
            }

            if (itu == out->uses.end()) 
                op_destroy_ssa(p);
        }
    }
}

void        funcdata::remove_calculated_loop(flowblock *lheader)
{
    vector<varnode *> outlist;

    if (lheader->loopnodes.size() != 2)
        throw LowlevelError("now only support 2 nodes constant loop remove");

    flowblock *pre = loop_pre_get(lheader, 0);
    flowblock *cur = lheader, *branch;
    pcodeop *p;
    list<pcodeop *>::iterator it;
    varnode *vn;

    do {
        int inslot = cur->get_in_index(pre), ret;
        branch = NULL;

        /* 因为这个计算可计算循环，不需要把节点重新拉出来，所以不加入trace列表 */
        for (it = cur->ops.begin(); it != cur->ops.end(); it++) {
            p = *it;

            p->set_trace();
            ret = p->compute(inslot, &branch);

            if ((vn = p->output) && (vn->type.height == a_top)) {
                /* FIXME: 我这里根据某些特殊情况对load做了特殊处理，让a_top类型的load值变成了
                0xbadbeef */
                if (p->opcode == CPUI_LOAD)
                    vn->set_val(0xbadbeef);
                else if (!vn->uses.empty())
                    throw LowlevelError("in calculated loop find unkown node");
            }

#if 0
            char buf[256];
            p->dump(buf, PCODE_DUMP_SIMPLE & ~PCODE_HTML_COLOR);
            printf("%s\n", buf);
#endif
        }

        pre = cur;
        cur = branch;
    } while (bblocks.in_loop(lheader, cur));

    remove_loop_livein_varnode(lheader);

    /* FIXME:以下的合并节点的方式不对，我简化处理了 */
    branch = lheader->loopnodes[1];
    if (!branch->ops.empty())
        throw LowlevelError("not support liveout ops live in loopsnode");

    bblocks.remove_edge(branch, lheader);
    bblocks.remove_edge(lheader, branch);
    bblocks.remove_block(branch);
    if ((lheader->out.size() == 1) && ((p = lheader->last_op())->opcode == CPUI_CBRANCH)) {
        op_destroy(p, 1);
    }

    for (it = lheader->ops.begin(); it != lheader->ops.end(); it++) {
        p = *it;
        p->clear_trace();
        vn = p->output;
        if (!vn) continue;
        if (vn->is_constant()) p->to_constant();
        if (vn->is_sp_constant()) p->to_rel_constant();
    }

    structure_reset();
}

void        funcdata::remove_calculated_loops()
{
    vector<flowblock *> loops;
    int i;

    detect_calced_loops(loops);

    for (i = 0; i < loops.size(); i++) {
        remove_calculated_loop(loops[i]);
    }

    heritage_clear();
    heritage();
}

/*

识别出这样一种结构 

top->b0
top->b1
b0->b1

b0里面只允许有1-2行代码 
*/
int         funcdata::cond_copy_propagation(varnode *phi)
{
    pcodeop *p = phi->def;
    flowblock *b0, *b1, *top;

    if (!p 
        || (p->opcode == CPUI_MULTIEQUAL)
        || (p->inrefs.size() != 2))
        throw LowlevelError("input varnode cant cond copy propagation");

    /* 模式匹配 */
    b0 = p->parent->get_in(0);
    b1 = p->parent->get_in(1);

    if (!b0->is_in(b1)) {
        if (!b1->is_in(b0))
            throw LowlevelError("cond_copy_propagation pattern mismatch");
        top = b1;
    }
    else
        top = b0;

    b0 = (top == b0) ? b1 : b0;
    b1 = p->parent;

    if (b0->ops.size() != 1)
        throw LowlevelError("cond_copy_propagation pattern mismatch");

    return 0;
}

int         funcdata::cond_copy_expand(pcodeop *p, flowblock *b, int outslot)
{
    flowblock *b1, *b2, *pb1, *pb2, *outb;
    vector<varnode *> defs, tdefs;
    int i, have_top, dfnum;
    varnode *def;
    assert(p->opcode == CPUI_COPY || p->opcode == CPUI_LOAD || p->opcode == CPUI_MULTIEQUAL);
    assert(outslot >= 0);
    outb = b->get_out(outslot);

    have_top = collect_all_const_defs(p, defs, tdefs, dfnum);

    if (defs.size() == 0)
        return -1;

    pb1 = pb2 = NULL;
    for (i = 0; i < defs.size(); i++) {
        def = defs[i];

        if ((def == defs.back()) && !have_top) {
            b1 = bblocks.new_block_basic(user_offset += user_step);
            pf.add_copy_const(b1, b1->ops.end(), *p->output, *defs[i]);

            bblocks.add_edge(pb1, b1);
            continue;
        }

        b1 = bblocks.new_block_basic(user_offset += user_step);
        pf.add_cmp_const(b1, b1->ops.end(), p->output, defs[i]);
        pf.add_cbranch_eq(b1);

        b2 = bblocks.new_block_basic(user_offset += user_step);
        pf.add_copy_const(b2, b2->ops.end(), *p->output, *defs[i]);

        bblocks.add_edge(b1, b2, a_true_edge);
        bblocks.add_edge(b2, outb);

        if (pb1)
            bblocks.add_edge(pb1, b1);

        pb1 = b1;
        pb2 = b2;

        if (i == 0) {
            int lab = bblocks.remove_edge(b, outb);
            bblocks.add_edge(b, b1, lab & a_true_edge);
        }
    }

    bblocks.add_edge(b1, outb);

    structure_reset();

    return 0;
}

int         funcdata::ollvm_do_copy_expand(pcodeop *p, flowblock *b, int outslot)
{
    vector<varnode *> defs, tdefs;
    int top, dfnum;

    /*
    我们处理以下情况的代码：

1.    v1 = -2117566407;
2.    if ( !((x_71 * (x_71 - 1)) << 31) )
3.        v1 = 1048196999;
4.    if ( y_72 < 10 )
5.        v1 = 1048196999;

    在这种情况下，没必要去展开指令5处的代码，因为v1的值其实就2个，直接做常数拷贝扩展。
    */
    if (p->opcode != CPUI_MULTIEQUAL) return -1;
    if (p->all_inrefs_is_constant()) return -1;
    if (p->inrefs.size() != 2) return -1;
    if (p->parent->out.size() != 1) return -1;

    top = collect_all_const_defs(p, defs, tdefs, dfnum);
    if (top)
        return -1;

    if (defs.size() == dfnum)  return -1;
    /* 我们只针对不透明的谓词处理一下，但是应该来说我们应该消除不透明谓词 */
    if (defs.size() > 4) return -1;

    cond_copy_expand(p, b, outslot);
    heritage_clear();
    heritage();

    return 0;
}

int         funcdata::collect_all_const_defs(pcodeop *start, vector<varnode *> &defs, vector<varnode *> &tdefs, int &dfnum, int keepsame)
{
    pcodeop *p, *def;
    pcodeop_set visit;
    pcodeop_set::iterator it;
    vector<pcodeop *> q;
    varnode *in;
    int i, j, have_top_def = 0, not_visited;

    dfnum = 0;

    q.push_back(start);
    visit.insert(start);
    while (!q.empty()) {
        p = q.front();
        q.erase(q.begin());

#define cad_push_def(in)        do { \
            def = (in)->def; \
            not_visited = (def && (visit.find(def) == visit.end())); \
            if (in->is_constant()) { \
                dfnum++; \
                if (!keepsame) { \
                    for (j = 0; j < defs.size(); j++) { \
                        if (defs[j]->get_val() == in->get_val()) break; \
                    } \
                    if (j == defs.size()) defs.push_back(in); \
                } \
                else \
                    defs.push_back(in); \
            } \
            else if (not_visited && ((def->opcode == CPUI_COPY) || (def->opcode == CPUI_MULTIEQUAL) || (def->opcode == CPUI_LOAD) || (def->opcode == CPUI_STORE))) { \
                q.push_back(def); \
                visit.insert(def); \
            } \
            else if (not_visited) { \
                tdefs.push_back(in); \
            } \
        } while (0)

        if (p->opcode == CPUI_COPY) {
            in = p->get_in(0);
            cad_push_def(in);
        }
        else if (p->opcode == CPUI_MULTIEQUAL) {
            for (i = 0; i < p->inrefs.size(); i++) {
                in = p->get_in(i);
                cad_push_def(in);
            }
        }
        else if ((p->opcode == CPUI_LOAD)) {
            if ((in = p->get_virtualnode()) && in->def && (in->def->opcode == CPUI_STORE)) {
                in = in->def->get_in(2);
                cad_push_def(in);
            }
            else
                tdefs.push_back(p->output);
        }
        else if ((p->opcode == CPUI_STORE)) {
            in = p->get_in(2);
            cad_push_def(in);
        }
        else
            tdefs.push_back(p->output);
    }

    return tdefs.size();
}

int         funcdata::cut_const_defs_on_condition(pcodeop *start, vector<varnode *> &defs)
{
    flowblock *b = start->parent;
    pcodeop *p[4];

    if (((p[0] = b->last_op())->opcode == CPUI_CBRANCH)
        && (p[1] = p[0]->get_in(0)->def)
        && (p[1]->opcode == CPUI_INT_EQUAL)) {
    }

    return 0;
}

void        funcdata::rewrite_no_sub_cbranch_blk(flowblock *b)
{
    list<pcodeop *>::iterator it, it1;
    flowblock *end, *newstart;
    vector<flowblock *> cloneblks;

    end = bblocks.get_it_end_block(b);
    while (end->is_it_cbranch())
        end = bblocks.get_it_end_block(end);

    clear_block_phi(b);
    clear_block_phi(end);

    while (b->in.size()) {
        flowblock *inb = b->get_in(0);

        cloneblks.clear();
        newstart = clone_web(b, end, cloneblks);

        bblocks.remove_edge(inb, b);
        bblocks.add_edge(inb, newstart);
    }
}

void        funcdata::rewrite_no_sub_cbranch_blks(vector<flowblock *> &blks)
{
    int i;

    for (i = 0; i < blks.size(); i++) {
        rewrite_no_sub_cbranch_blk(blks[i]);
    }

    remove_unreachable_blocks(true, true);
    structure_reset();
    heritage_clear();
    heritage();
}


int         funcdata::ollvm_combine_lcts(pcodeop *p)
{
    assert(p->opcode == CPUI_COPY);
    varnode *in = p->get_in(0);
    flowblock *b = p->parent, *tob, *b1;
    list<pcodeop *>::iterator it;
    pcodeop *p1;
    vector<flowblock *> blks;

    if (b->out.size() != 1) return 0;

    tob = b->get_out(0);

    for (it = in->uses.begin(); it != in->uses.end(); it++) {
        p1 = *it;
        b1 = p1->parent;

        if ((b1->out.size() != 1) || (b1->get_out(0) != tob))
            continue;

        blks.push_back(b1);
    }

    if (blks.size() < 2) return 0;
    if (blks[0] == blks[1]) return 0;

    return combine_lcts(blks);
}

int         funcdata::combine_lcts(vector<flowblock *> &blks)
{
    flowblock *b = blks[0], *tob;
    vector<list<pcodeop *>::reverse_iterator> its;
    vector<pcodeop *> ops;
    list<pcodeop *>::reverse_iterator it;
    pcodeop *p, *p1;
    int i, len, end = 0;

    its.resize(blks.size());
    tob = b->get_out(0);
    for (i = 0; i < blks.size(); i++) {
        b = blks[i];
        /* 所有节点的出口节点必须一致，且只有一个出口节点 */
        if (b->ops.size() == 0) return 0;
        if (b->out.size() != 1) return 0;
        if (b->get_out(0) != tob) return 0;

        its[i] = b->ops.rbegin();
    }

    b = NULL;
    while (!b) {
        p = *(its[0]);
        for (i = 1; i < its.size(); i++) {
            p1 = *(its[i]);

            if (pcodeop_struct_cmp(p, p1, 0)) break;
        }

        if (i == its.size())
            ops.insert(ops.begin(), p);
        else {
            /* 


            blockA:
            0x18200: ldrd r4,r9,[sp,#0x288]
            p74014 [239]:(u8fc00.1152:4) = INT_ADD (%sp.3:4) (#288:4)
            p74015 [240]:(%r4.240:4) = LOAD ram (u8fc00.1152:4) (s-170.113:4)
            p74016 [241]:(u97080.375:4) = INT_ADD (u8fc00.1152:4) (#4:4)
            p74017 [242]:(%r9.174:4) = LOAD ram (u97080.375:4) (s-16c.67:4)

            blockB:
            0x18200: ldrd r4,r9,[sp,#0x288]
            p344636 [ 80]:(u8fc00.786:4) = INT_ADD (%sp.3:4) (#288:4)
            p344638 [ 81]:(u97080.264:4) = INT_ADD (u8fc00.786:4) (#4:4)
            p344639 [ 82]:(%r9.115:4) = LOAD ram (u97080.264:4) (s-16c.44:4)

            这个块，其实它们本来是一样的，但是因为优化的原因，导致其中blockB少了一条指令，
            然后我们在做块合并时，要基于指令单位来做，发现这个情况后，要退掉尾部那些同一个
            指令产生的pcode
            */
            while (ops.size() && (ops.front()->get_addr() == p->get_addr())) {
                for (i = 0; i < its.size(); i++)
                    its[i]--;
                ops.erase(ops.begin());
            }
            break;
        }

        for (i = 0; i < its.size(); i++) {
            its[i]++;
            if (its[i] == blks[i]->ops.rend()) b = blks[i];
        }
    }

    if ((ops.size() == 0) || ((ops.size() == 1) && (ops.back()->opcode == CPUI_BRANCH))) return 0;

    /*
    合并前驱节点的公共尾部子串时，假如某个前驱刚好满足交集，直接使用整个前驱
    */
    if (!b) {
        b = bblocks.new_block_basic(user_offset += user_step);

        for (i = 0; i < ops.size(); i++) {
            pcodeop *p1 = ops[i];
            Address addr2(d->getDefaultCodeSpace(), p1->get_addr().getOffset());
            const SeqNum sq(addr2, op_uniqid++);
            p = cloneop(p1, sq);
            op_insert_end(p, b);
        }

        bblocks.add_edge(b, tob);
    }

    len = ops.size();
    while (len--) {
        for (i = 0; i < its.size(); i++) {
            its[i]--;

            if ((*(its[i]))->parent == b) continue;

            op_destroy_ssa(*(its[i]));
        }
    }

    for (i = 0; i < blks.size(); i++) {
        if (blks[i] != b) {
            bblocks.remove_edge(blks[i], tob);
            bblocks.add_edge(blks[i], b);
        }
    }

    structure_reset();
    clear_block_phi(tob);

    return 1;
}

int         funcdata::combine_lcts_blk(flowblock *b)
{
    flowblock *inb;
    pcodeop *lastop;
    int i, j, ret = 0;
    vector<vector<flowblock *>> blks_list;

    if (b->in.size() <= 1) return 0;

    for (i = 0; i < b->in.size(); i++) {
        inb = b->get_in(i);
        lastop = inb->last_op();

        if (inb->out.size() != 1) continue;

        /* 查找是否有相同pcode的blk，找到就退出 */
        for (j = 0; j < blks_list.size(); j++) {
            if (pcodeop_struct_cmp(blks_list[j][0]->last_op(), lastop, 0) == 0) break;
        }

        if (j == blks_list.size()) {
            /* 没找到相同的blk，创建一个新的vector */
            blks_list.push_back(vector<flowblock *>());
            blks_list.back().push_back(inb);
        }
        else {
            /* 找到了相同的blk，放入一个数组 */
            blks_list[j].push_back(inb);
        }
    }

    for (i = 0; i < blks_list.size(); i++) {
        if (blks_list[i].size() >= 2)
            ret |= combine_lcts(blks_list[i]);
    }

    return ret;
}

int         funcdata::combine_lcts_all(void)
{
    int i, size = bblocks.blist.size(), lcts;

    do {
        lcts = 0;
        for (i = 0; i < bblocks.blist.size(); i++) {
            lcts |= combine_lcts_blk(bblocks.blist[i]);
        }
        
        if (lcts) {
            do
            {
                cond_constant_propagation();
                dead_code_elimination(bblocks.blist, 0);
            } while (!cbrlist.empty() || !emptylist.empty());
        }
    } while (lcts);

    return 0;
}

int         funcdata::cmp_itblock_cbranch_conditions(pcodeop *cbr1, pcodeop* cbr2)
{
    list<pcodeop *>::iterator it1 = cbr1->insertiter;
    list<pcodeop *>::iterator it2 = cbr2->insertiter;

    pcodeop *p1 = *--it1;
    pcodeop *pp1 = *--it1;

    pcodeop *p2 = *--it2;
    pcodeop *pp2 = *--it2;

    /* 这个复杂的表达式，只是模拟ssa的效果，保证du链，确保我们是在比较同一个东西，因为这个时候ssa还没有计算 */
    if ((p1->opcode == CPUI_BOOL_NEGATE) && (p2->opcode == CPUI_BOOL_NEGATE) && (pp1->opcode == pp2->opcode)
        && p1->output->get_addr() == cbr1->get_in(1)->get_addr()
        && pp1->output->get_addr() == p1->get_in(0)->get_addr()
        && p2->output->get_addr() == cbr2->get_in(1)->get_addr()
        && pp2->output->get_addr() == p2->get_in(0)->get_addr()
        && (pp1->get_in(0)->get_addr() == pp2->get_in(0)->get_addr())) {
        return 0;
    }

    return -1;
}

bool        funcdata::vmp360_detect_vmeip()
{
    flowblock *vmhead = get_vmhead(), *exit = NULL;
    varnode *iv = detect_induct_variable(vmhead, exit), *pos;
    pcodeop *op, *def;
    char buf[128];

    if (!iv)
        throw LowlevelError("vmp360 not found VMEIP");

    op = vmhead->find_pcode_def(iv->get_addr());

    /**/
    for (int i = 0; i < op->inrefs.size(); i++) {
        /* 检测到归纳变量以后(也就是vmeip)，扫描它从哪个边进来的，假如不是来自于回边，直接退出 */
        if (!(vmhead->in[i].label & a_back_edge)) continue;

        def = op->get_in(i)->def;
        if (def->opcode != CPUI_LOAD) continue;
        pos = def->get_in(1);
        if (pos->type.height == a_sp_constant) {
            print_varnode(d->trans, buf, pos);

            printf("****found VMEIP [%s]\n", buf);
            vmeip = pos->get_val();
            return true;
        }
    }

    return false;
}

int         funcdata::vmp360_detect_safezone()
{
    if (vmeip == -1)
        throw LowlevelError("vmp360_detect_safezone need vmeip detect");

    /* FIXME:硬编码一波 

    */

    set_safezone(vmeip - 0x24, 0xa0);

    return 0;
}

int         funcdata::vmp360_detect_framework_info()
{
    if (!vmp360_detect_vmeip())
        throw LowlevelError("vmp360_detect_vmeip not found vmeip");

    if (vmp360_detect_safezone())
        throw LowlevelError("vmp360_detect_safezone() failure ");

    return 0;
}

int         funcdata::ollvm_deshell()
{
    int i;
    unsigned int tick = mtime_tick();
    char buf[16];
    flowblock *h;

    //flags.disable_to_const = 1;
    flags.disable_simd_to_const = 1;
    flags.disable_inrefs_to_const = 1;
    follow_flow();
    heritage();

    dump_cfg(name, "orig0", 1);

    if (!is_complete_function())
        try_to_completion_function();

    flags.enable_peephole = 1;
    heritage_clear();
    heritage();

    while (!cbrlist.empty() || !emptylist.empty()) {
        cond_constant_propagation();
        dead_code_elimination(bblocks.blist, 0);
    }

    dump_cfg(name, "orig1", 1);

    ollvm_detect_frameworkinfo();

    dump_cfg(name, "orig", 1);

    h = ollvm_get_head();
    for (i = 0; loop_dfa_connect(0) >= 0; i++)
    {
        print_tag("[%s] loop_unrolling sub_%llx %d times*********************** \n\n", mtime2s(NULL),  h->get_start().getOffset(), i);

        dead_code_elimination(bblocks.blist, RDS_UNROLL0);

        if (d->debug.dump_cfg)
            dump_cfg(name, _itoa(i, buf, 10), 1);
    }

    /* 删除所有splice 模块，不会导致ssa关系重构 */
    bblocks.clear_all_unsplice();
    bblocks.clear_all_vminfo();

    remove_dead_stores();
    dead_code_elimination(bblocks.blist, F_REMOVE_DEAD_PHI);

    do {
        cond_constant_propagation();
        dead_code_elimination(bblocks.blist, 0);
    } while (!cbrlist.empty() || !emptylist.empty());

    //dead_code_elimination(bblocks.blist, F_REMOVE_DEAD_PHI);

#if 0
    dump_cfg(name, "before_lcts", 1);
    heritage_clear();
    heritage();
#endif

    combine_lcts_all();

#if 0
    dump_cfg(name, "final0", 1);
    heritage_clear();
    heritage();

    do {
        cond_constant_propagation();
        dead_code_elimination(bblocks.blist, 0);
    } while (!cbrlist.empty() || !emptylist.empty());
#endif

    dump_cfg(name, "final", 1);

	print_tag("de-ollvm spent %u ms\n", mtime_tick() - tick);

    dump_exe();

    return 0;
}

int         funcdata::static_trace(pcodeop *op, int inslot, flowblock **branch)
{
    op->set_trace();
    valuemap::iterator it = tracemap.find(op);
    static valuetype empty;

    if (it == tracemap.end()) {
        if (op->output)
            tracemap[op] = op->output->type;
        else
            tracemap[op] = empty;
    }

    return op->compute(inslot, branch);
}

void        funcdata::static_trace_restore()
{
    valuemap::iterator it = tracemap.begin();
    pcodeop *p;

    for (; it != tracemap.end(); it++) {
        p = it->first;

        p->clear_trace();
        if (p->output)
            p->output->type = tracemap[p];
    }

    tracemap.clear();
}

bool        funcdata::is_safe_sp_vn(varnode *vn) 
{
    return (vn->get_addr().getSpace() == d->getStackBaseSpace()) && (in_safezone(vn->get_addr().getOffset(), vn->size));
}

bool        funcdata::use_old_inst(vector<pcodeop *> &plist)
{
    Address addr = plist[0]->get_addr();
    vector<pcodeop_lite *> &oldlist(litemap[addr]);
    int i, j;

    if (oldlist.size() == 0) return false;

    for (i = j = 0; i < oldlist.size(); i++) {
        pcodeop_lite *lite = oldlist[i];

        if (lite->have_side_effect()) return false;

        /* SIMD? */
        int reg = d->reg2i(poa(lite));
        /* 非标准寄存器变量 */
        if (reg == -1) continue;

        for (j = 0; j < plist.size(); j++) {
            if (poa(plist[j]) == poa(lite))
                break;
        }

        /* 假如没有在最新的inst-pcode 列表中找到老的对应的寄存器，而且出口也不为死，则没有匹配上，直接报错  */
        if ((j == plist.size()) && plist.back()->live_out[reg])
            return false;
    }

    return true;
}

int        funcdata::vmp360_deshell()
{
	unsigned int tick = mtime_tick();

    follow_flow();

    inline_call("", 1);
    inline_call("", 1);

    /* 这里需要heritage一次，才能扫描到 vmp360的框架信息，
    而没有这一次vmp360的框架信息扫描，它们后面的 heritage也是不完全的，所以在检测完360的框架以后
    需要再来一次heritage*/
    heritage();
    if (vmp360_detect_framework_info())
        throw LowlevelError("vmp360_deshell not found vmeip");

	heritage_clear();
    heritage();

    if (!cbrlist.empty())
        cond_constant_propagation();

    remove_calculated_loops();
    if (!cbrlist.empty())
        cond_constant_propagation();

	flags.enable_topstore_mark = 1;
    dump_cfg(name, "orig1", 1);

    int i;
    for (i = 0; get_vmhead(); i++) {
    //for (i = 0; i < 3; i++) {
        printf("loop unrolling %d times*************************************\n", i);
        loop_unrolling4(get_vmhead(), i, _NOTE_VMBYTEINDEX);
        dead_code_elimination(bblocks.blist, RDS_UNROLL0);
#if defined(DCFG_CASE)
        dump_cfg(name, to_string(i), 1);
#endif
    }

    dump_exe();

    dump_cfg(name, "final", 1);
    dump_pcode("1");
    dump_djgraph("1", 1);
    //fd_main->dump_phi_placement(17, 5300);
    dump_store_info("1");
    dump_loop("1");

	printf("deshell spent %u ms\n", mtime_tick() - tick);

    return 0;
}


bool        funcdata::is_stack_balance()
{
    int i;

    for (i = 0; i < bblocks.exitlist.size(); i++) {
        pcodeop *lastop = bblocks.exitlist[i]->last_op();
        if (lastop->is_call()) {
            /* 假如最后的退出函数，是noreturn，那么即使堆栈不平衡，我们也认为平衡 */
            if (lastop->callfd && !lastop->callfd->noreturn())
                return false;
        }
        /* 假如最后的退出节点，堆栈不平衡，那么返回false */
        else if (lastop->sp)
            return false;
    }

    return true;
}

void        funcdata::generate_sp_info()
{
    list<pcodeop *>::iterator it;
    vector<flowblock *> q;
    flowblock *b = bblocks.get_entry_point(), *outb;
    int sp = 0;

    b->set_mark();
    q.push_back(b);

    while (!q.empty()) {
        b = q.front();
        q.erase(q.begin());

        sp = b->sp;
        for (it = b->ops.begin(); it != b->ops.end(); it++) {
            pcodeop *p = *it;

            p->sp = sp;
            varnode *o = p->output;
            if (o && o->get_addr() == d->sp_addr) {
                if (o->is_sp_constant())
                    sp = -o->get_val();
                else
                    sp = INT_MAX;
            }
        }

        for (int i = 0; i < b->out.size(); i++) {
            outb = b->get_out(i);
            if (outb->is_mark()) continue;

            outb->set_mark();
            q.push_back(outb);
            outb->sp = sp;
        }
    }

    bblocks.clear_marks();
}

int         funcdata::try_to_completion_function()
{
    if (is_stack_balance()) return 0;
    int loop = 0;

    while (!is_complete_function()) {
        for (int i = 0; i < bblocks.exitlist.size(); i++) {
            flowblock *outb = bblocks.exitlist[i];

            if (!outb->is_dyn_reachable()) continue;

            pcodeop *lastop = outb->last_op();

            if ((lastop->opcode == CPUI_BRANCHIND) && lastop->get_in(0)->is_pc_constant()) {
                addrlist.push_back(Address(d->getDefaultCodeSpace(), lastop->get_in(0)->get_val()));

                generate_ops();
            }
        }

        phi_clear();
        heritage_clear();

        generate_blocks();
        heritage();

        dump_cfg(name, "complete" + to_string(loop), 1);
        loop++;
    }

    exit(0);

    return 0;
}

bool        funcdata::is_complete_function()
{
    int i;

    bblocks.mark_dynamic_reachability();

    for (i = 0; i < bblocks.exitlist.size(); i++) {
        flowblock *b = bblocks.exitlist[i];
        pcodeop *p = b->last_op();

        if (!b->is_dyn_reachable()) continue;

        if (p->flags.exit) continue;

        if (p->callfd && !p->callfd->noreturn())
            return false;

        if ((p->opcode != CPUI_RETURN))
            return false;
    }

    return true;
}

int         funcdata::zr0_lead_to_unreachable_edge(pcodeop *op, blockedge *&e)
{
    pcodeop *p;
    if (poa(op) == d->tzr_addr) {
        p = op->output->uses.front();
        if (poa(p) == d->zr_addr) {
            p = p->output->uses.front();
            if (p && (p->opcode == CPUI_INT_NOTEQUAL) && p->get_in(1)->is_val(0)) {
                p = p->output->uses.front();
                if (p && (p->opcode == CPUI_CBRANCH)) {
                    e = p->parent->get_true_edge();
                    return 0;
                }
            }
        }
    }

    return -1;
}
