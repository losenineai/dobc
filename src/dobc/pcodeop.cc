
#include "dobc.hh"
#include "pcodeop.hh"
#include "thumb_gen.hh"
#include "high.hh"

#define MSB4(a)                 (a & 0x80000000)
#define MSB2(a)                 (a & 0x8000)

#define zext(in,insiz,outsiz)               in

#define VAL_MASK(v,m)      (v & (((uintb)1 << (m * 8)) - 1))

intb sext(intb in, int insiz, int outsz)
{
    if (in & ((intb)1 << (insiz - 1)))
        return (~(((intb)1 << insiz) - 1)) | in;

    return in;
}

pcodeop::pcodeop(int s, const SeqNum &sq)
    :start(sq), inrefs(s)
{
    memset(&flags, 0, sizeof(flags));
    flags.dead = 1;
    parent = 0;

    output = 0;
    opcode = CPUI_MAX;
}
pcodeop::~pcodeop()
{
}

void    pcodeop::set_opcode(OpCode op)
{
    if (opcode != CPUI_MAX)
        flags.changed = 1;

    opcode = op;

    /* FIXME:后面要切换成typefactory，参考Ghidra */
    if (opcode == CPUI_CBRANCH || opcode == CPUI_BRANCH || opcode == CPUI_CALL)
        flags.coderef = 1;
}

void            pcodeop::clear_input(int slot) 
{
    inrefs[slot] = NULL; 
}

void    pcodeop::remove_input(int slot)
{
    for (int i = slot + 1; i < inrefs.size(); i++)
        inrefs[i - 1] = inrefs[i];

    inrefs.pop_back();
}

void    pcodeop::insert_input(int slot)
{
    int i;
    inrefs.push_back(NULL);
    for (i = inrefs.size() - 1; i > slot; i--)
        inrefs[i] = inrefs[i - 1];
    inrefs[slot] = NULL;
}

// 扩展outbuf的内容区，用来做对齐用 
#define expand_line(num)        while (i < num) buf[i++] = ' '
int             pcodeop::dump(char *buf, uint32_t flags)
{
    int i = 0, j, in_limit = 10000;
    dobc *d = parent->fd->d;
    Translate *trans = parent->fd->d->trans;

    i += sprintf(buf + i, " p%-3d [%3d]:", start.getTime(), start.getOrder());

    if (output) {
        i += print_varnode(trans, buf + i, output);
        i += sprintf(buf + i, " = ");
    }

    i += sprintf(buf + i, "%s", get_opname(opcode));
    // Possibly check for a code reference or a space reference

    if (flags & PCODE_OMIT_MORE_IN) in_limit = 4;

    if (callfd) 
        i += sprintf(buf + i, ".%s ", callfd->name.c_str());

    j = 0;
    if (opcode == CPUI_CALLOTHER) {
        i += sprintf(buf + i, ".%s", d->get_userop_name(inrefs[0]->get_addr().getOffset()).c_str());
        j = 1;
    }

    for (; j < inrefs.size(); ++j) {
        if (j == in_limit) break;
        i += sprintf(buf + i, " ");
        i += print_varnode(trans, buf + i, inrefs[j]);
    }

    if (j == in_limit)
        i += sprintf(buf + i, "[...]");

    expand_line(48);

    if (flags & PCODE_DUMP_VAL) {
        if (flags & PCODE_HTML_COLOR)   i += sprintf(buf + i, "<font color=\"red\"> ");
        i += print_vartype(trans, buf + i, output);
        if (flags & PCODE_HTML_COLOR)   i += sprintf(buf + i, " </font>");
    }

    if (flags & PCODE_DUMP_UD)
        i += print_udchain(buf + i, this, flags);

    if (is_call() || (opcode == CPUI_USE)) {
        if (flags & PCODE_HTML_COLOR)   i += sprintf(buf + i, "<font color=\"red\"> ");

        i += sprintf(buf + i, "[sp:");
        i += print_vartype(trans, buf + i, get_in(d->sp_addr));
        i += sprintf(buf + i, "]");
        if (flags & PCODE_HTML_COLOR)   i += sprintf(buf + i, " </font>");
    }

    if (this->flags.vm_eip) {
        i += sprintf(buf + i, "<font color=\"green\">**VMEIP</font>");
    }

    buf[i] = 0;

    return i;
}

bool            pcodeop::in_sp_alloc_range(varnode *pos)
{
    if (opcode != CPUI_INT_SUB)  return false;

    Address &sp = parent->fd->d->sp_addr;

    varnode *in0 = get_in(0);
    varnode *in1 = get_in(1);

    if (output && output->is_sp_constant()
        && in0->is_sp_constant()
        && in1->is_constant()
        && pos->is_sp_constant()
        && (-pos->get_val() > -in0->get_val()) 
        && (-pos->get_val() <= -output->get_val())) {
        return true;
    }

    return false;
}

void            pcodeop::on_MULTIEQUAL()
{
    varnode *vn, *cn = NULL, *vn1;
    vector<varnode *> philist;
    set<intb> c;
    pcodeop_set visit;
    pcodeop_set::iterator it;
    pcodeop *p1, *p;
    int i;

    visit.insert(this);

    for (i = 0; i < inrefs.size(); i++) {
        vn = get_in(i);
        p = vn->def;
        if (vn->is_constant()) {
            c.insert(vn->get_val());
            cn = vn;
            continue;
        }
        else if (!p) {
            output->set_top();
            return;
        }
        else if (p->opcode != CPUI_MULTIEQUAL) {
            p = vn->search_copy_chain(CPUI_MULTIEQUAL, NULL);

            if (!p || (p->opcode != CPUI_MULTIEQUAL)) {
                output->set_top();
                return;
            }
        }

        if (p->opcode == CPUI_MULTIEQUAL) {
            it = visit.find(p);
            if (it == visit.end())
                philist.push_back(p->output);
        }
    }

    /* 假如有2个常量定值进来，那么值的height必然是T */
    if (c.size() > 1) {
        output->type.height = a_top;
        return;
    }

    /* 假如所有的常量的值，完全一样，而且philist节点为空，那么设置phi节点的out值为常量*/
    if (philist.size() == 0) {
        output->type = cn->type;
        return;
    }

    /* 
    1. 假如一个phi节点中，所有常量的值相等
    2. 另外的非常量节点都是Phi节点
    3. 递归的遍历其中的phi节点，重新按1,2开始
    4. 遍历完成后，那么此phi节点也是常量
    */
    while (!philist.empty()) {
        vn = philist.front();
        philist.erase(philist.begin());

        p = vn->def;
        it = visit.find(p);
        if (it != visit.end()) continue;

        visit.insert(p);

        for (i = 0; i < p->inrefs.size(); i++) {
            vn1 = p->get_in(i);
            p1 = vn1->def;

            if (vn1->is_constant()) {
                if (!cn) cn = vn1;
                else if (vn1->type != cn->type) {
                    goto top_label;
                }
                continue;
            }
            else if (!p1) {
                goto top_label;
            }
            else if (p1->opcode != CPUI_MULTIEQUAL) {
                p1 = vn1->search_copy_chain(CPUI_MULTIEQUAL, NULL);

                if (!p1 || (p1->opcode != CPUI_MULTIEQUAL))
                    goto top_label;
            }

            if (p1->opcode == CPUI_MULTIEQUAL) {
                it = visit.find(p1);
                if (it == visit.end()) {
                    philist.push_back(p1->output);
                }
            }
        }
    }

    if (cn) {
        output->type = cn->type;
        return;
    }

top_label:
    if (on_cond_MULTIEQUAL2())
        output->set_top();
}

/*
我们主要处理这么几种情况:

情况1:
      v15 = -1595496483;
      if ( y >= 10 )
        v15 = -294400669;
      v16.0 = -294400669;
      if ( y >= 10 )
        v16.1 = v15;
      v16.2=phi(v16.0, v16.1)
      if ( v16.2 != -294400669 )

      v16.2 的值一定等于 -294400669

情况2:
      v11.0 = 373666521;
      if ( y >= 10 )
        v11.1 = 44567630;
      v11.2 = phi(v11.0, v11.1)
      v14 = 0;
      if ( y >= 10 )
        v11.3 = 373666521;
      v11.4 = phi(v11.2, v11.3)

      v11的值等于 373666521

情况3:
      v18.0 = 2040621544;
      v19.0 = 0;
      if ( y.0 >= 10 )
        v18.1 = -1595496483;
      v18.2 = phi(v18.0, v18.1)
      if ( y < 10 )
        v19.1 = 1;
      v19.2 = phi (v19.0, v19.1)
      if ( v19.2 != 1 )
        v18.3 = 2040621544;
      v18.4 = phi(v18.2, v18.3)
*/
#define TOP             1
int             pcodeop::on_cond_MULTIEQUAL2()
{
    /* 调试用 */
    //if (!dobc::singleton()->debug.open_phi2) return TOP;

    funcdata *fd = parent->fd;
    pcodeop *topp;
    vector<varnode *> defs;
    int ret, dfnum;

    if ((parent->in.size() != 2) || all_inrefs_is_constant() || all_inrefs_is_top() )
        return TOP;

    ret = fd->collect_all_const_defs(this, defs, dfnum);
    /* 假如当前phi节点的常量定义都是不一样的，则不计算  */
    if (ret || (defs.size() == dfnum))
        return ret;

    /* 当phi节点的部分常量定义是一样的，则有可能去重 */
    varnode *constvn = get_const_in();
    varnode *topvn = get_top_in(), *c1;
    high_cond topcond, cond;
    flowblock *b = this->parent, *topb, *topb1, *topb2, *b1;

    topp = topvn->def;
    if (topp->opcode == CPUI_COPY)
        topp = topp->get_in(0)->def;

    if (!fd->is_ifthenfi_structure(topb = b->get_min_dfnum_in(), b1 = b->get_max_dfnum_in(), b)
        || !fd->is_ifthenfi_structure(topb1 = topb->get_min_dfnum_in(), topb->get_max_dfnum_in(), topb)
        || topb->update_cond()
        || topb1->update_cond())
        return TOP;

    c1 = topp->get_const_in(constvn);

    /* 我们确认top变量，是从哪个条件泄露进来的 */
    topcond.update(topb, (b1 == topvn->def->parent) ? b1:b);

    if (!c1)
        return TOP;

    /* FIXME:调试用 */
    static int trace = 0;
    trace++;

    if (topb->cond == topb1->cond) {
        cond.update(topb1, c1->def->parent);

        if (topcond == cond) {
            output->set_val(c1->get_val());
            return 0;
        }

        return TOP;
    }
    else if (!fd->is_ifthenfi_structure(topb2 = topb1->get_min_dfnum_in(), topb1->get_max_dfnum_in(), topb1))
        return TOP;

    topb2->update_cond();

    topb2->cond.linkto(topb1->cond);
    topb1->cond.linkto(topb->cond);


    cond.update(topp->parent->get_min_dfnum_in(), c1->def->parent);

    if (topcond == cond) {
        output->set_val(c1->get_val());
        return 0;
    }

    return TOP;
}

int             pcodeop::on_cond_MULTIEQUAL()
{
    funcdata *fd = parent->fd;

    return -1;

    if (inrefs.size() != 2) 
        return -1;

    varnode *constvn = get_const_in();
    varnode *topvn = get_top_in(), *condvn, *vn, *vn1;
    pcodeop *topp, *condop;
    flowblock *topb, *constb, *topb1, *topb2, *if2;

    if (!(topp = topvn->def) || !constvn || !constvn->def || (topp->inrefs.size() != 2) || !topp->all_inrefs_is_constant())
        return -1;

    topb1 = topp->parent;

    /* 保证这个phi 处于这样一个结构中:
        topb2
       /    \
      if2    /
       \    /
        topb1
        /   \
      xorb  /
       \   /
        topb
       /   \
    constb /
       \  /
      this(parent)
    */

    if (!fd->is_ifthenfi_structure((topb = topp->parent), constb = constvn->def->parent, parent)
        || topb->in.size() != 2
        || !(topb1 = topb->get_min_dfnum_in())
        || !fd->is_ifthenfi_structure(topb1,  topb1->get_cbranch_xor_out(topb), topb)
        || (topb1->in.size() != 2)
        || !(topb2 = topb1->get_min_dfnum_in())
        || !fd->is_ifthenfi_structure(topb2, if2 = topb2->get_cbranch_xor_out(topb1), topb1))
        return -1;

    int cond = topb->get_cbranch_cond();

    if (cond == COND_EQ) {
        topp = topvn->def;

        pcodeop *zrp = topb->get_last_oper_zr();
        pcodeop *core;
        if (zrp->opcode == CPUI_COPY) {
            zrp = zrp->get_in(0)->def;
            core = zrp->get_in(0)->def;
            if (zrp->get_in(1)->is_constant() && (zrp->get_in(1)->get_val() == 0)
                && (core->opcode == CPUI_INT_XOR) && (pi1(core)->get_val() == 0)
                && (condop = (condvn = pi0(core))->def)
                && (condop->opcode == CPUI_MULTIEQUAL)
                && (condop->all_inrefs_is_constant())
                && topb1->have_same_cmp_condition(topb2)
                && (vn = topb2->get_false_vn(condop))
                && topb->lead_to_false_edge(core, condop, vn)
                && (vn = topb2->get_true_vn(condop))
                && topb->lead_to_true_edge(core, condop, vn)
                && (vn = topb->get_false_vn(this))
                && vn->is_constant()
                && (vn1 = topb1->get_true_vn(topp))
                && vn1->is_constant()
                && vn->get_val() == vn1->get_val()) {
                output->set_val(vn->get_val());
                return 0;
            }
        }
    }

    return -1;
}


void            pcodeop::loadram2out(Address &addr)
{
    dobc *d = parent->fd->d;
    unsigned char buf[8];
    varnode *out = output;
    int ret;

    memset(buf, 0, sizeof(buf));
    ret = d->loader->loadFill(buf, out->size, addr);
    if (ret == DATA_TOP) {
        out->set_top();
        return;
    }

    if (out->size == 1)
        out->set_val(*(int1 *)buf);
    else if (out->size == 2)
        out->set_val(*(int2 *)buf);
    else if (out->size == 4)
        out->set_val(*(int *)buf);
    else if (out->size == 8)
        out->set_val(*(intb *)buf);
}

void            pcodeop::create_stack_virtual_vn()
{
    funcdata *fd = parent->fd;
    dobc *d = parent->fd->d;
    varnode *vn;
    if ((opcode != CPUI_LOAD) && (opcode != CPUI_STORE)) return;

    if ((opcode == CPUI_LOAD) && !get_virtualnode()) {
        if (!get_virtualnode() && get_in(1)->is_sp_constant()) {
            Address addr(d->getStackBaseSpace(),  get_in(1)->get_val() + STACK_BASE);
            vn = fd->create_vn(output->size, addr);
            fd->op_set_input(this, vn, 2);
        }
    }
    else {
        if (!output) {
            Address addr(d->getStackBaseSpace(),  get_in(1)->get_val() + STACK_BASE);
            vn = fd->create_vn(get_in(2)->size, addr);
            vn->version = ++fd->vermap[addr];
            fd->op_set_output(this, vn);
        }
    }
}

pcodeop*        pcodeop::find_same_pos_load(vector<pcodeop *> &storelist)
{
    pcodeop *p = this;

    while (p) {
        switch (p->opcode) {
        case CPUI_LOAD:
            if (!p->get_virtualnode()) return NULL;

            if (p->get_in(2)->get_addr() == output->get_addr())
                return p;

            p = p->get_in(2)->def;
            break;

        case CPUI_STORE:
            storelist.push_back(p);
            p = p->get_in(2)->def;
            break;

        default:
            return NULL;
        }
    }

    return NULL;
}

bool            pcodeop::all_inrefs_is_constant(void)
{
    int i;

    for (i = 0; i < inrefs.size(); i++) {
        if (!inrefs[i]->is_constant()) return false;
    }
    return true;
}

bool            pcodeop::all_inrefs_is_top(void)
{
    int i;

    for (i = 0; i < inrefs.size(); i++) {
        if (!inrefs[i]->is_top()) return false;
    }

    return true;
}

bool            pcodeop::all_inrefs_is_adj(void)
{
    int i;

    for (i = 0; i < inrefs.size(); i++) {
        pcodeop *p = inrefs[i]->def;
        if (!p) return false;
        if (parent->is_adjacent(p->parent)) continue;
        return false;
    }
    return true;
}

int				pcodeop::compute_add_sub()
{
	varnode *in0, *in1, *_in0, *_in1, *out;
	pcodeop *op;
	funcdata *fd = parent->fd;

	in0 = get_in(0);
	in1 = get_in(1);
	out = output;

	op = in0->def;

	if (!is_trace() && !(dobc::singleton()->is_simd(get_addr())) && op && ((op->opcode == CPUI_INT_ADD) || (op->opcode == CPUI_INT_SUB))) {
		_in0 = op->get_in(0);
		_in1 = op->get_in(1);

		/*
		ma = ma + 4;
		x = ma + 4;

		转换成
		x = ma + 8;

		关于活跃范围判断有2种情况

		1. r0 = r1 + 4
		2. r1 = r4 + 4
		3. sp = r0 + 4

		假如你想优化 sp = r1 + 4，需要判断 指令3 是不是在r1的活动范围内

		1. ma = ma + 4
		2. sp = ma + 4

		当前r0, r1位置的寄存器相等时，需要判断ma的范围
		*/
        while (
            _in0->is_sp_constant()
            && !dobc::singleton()->is_simd(op->get_addr())
			&& (in0->uses.size() == 1) && _in1->is_constant() 
			&& ((op->output->get_addr() == _in0->get_addr()) || _in0->in_liverange_simple(this))) {
			intb v = in1->get_val();

			if (opcode == CPUI_INT_ADD) {
				if (op->opcode == CPUI_INT_ADD)
					v = _in1->get_val() + v;
				else
					v = -_in1->get_val() + v;
			}
			else {
				if (op->opcode == CPUI_INT_ADD)
					v = -_in1->get_val() + v;
				else
					v = _in1->get_val() + v;
			}

			while (num_input() > 0)
				fd->op_remove_input(this, 0);

			fd->op_set_input(this, in0 = _in0, 0);
			fd->op_set_input(this, in1 = fd->create_constant_vn(v, in1->size), 1);
			fd->op_destroy(op);

			op = _in0->def;
			if ((op->opcode != CPUI_INT_ADD) && (op->opcode != CPUI_INT_SUB))
				break;
			_in0 = op->get_in(0);
			_in1 = op->get_in(1);
		}
	}

	if (opcode == CPUI_INT_ADD)
		out->set_sp_constant(in0->type.v + in1->type.v);
	else
		out->set_sp_constant(in0->type.v - in1->type.v);

	return 0;
}

int             pcodeop::compute_on_or(void)
{
    pcodeop *op, *op1;
    varnode *in0, *in1;

    in0 = get_in(0);
    in1 = get_in(1);

    if (in0->is_constant() && in1->is_constant()) {
        output->set_val(in0->get_val() | in1->get_val());
        return 0;
    }

    /* 
    x & 3
    */
    if ((op = in0->def) && (op1 = in1->def)
        && (op->opcode == CPUI_INT_AND) && (op1->opcode == CPUI_INT_AND)) {
    }

    output->set_top();

    return 0;
}

int             pcodeop::compute(int inslot, flowblock **branch)
{
    varnode *in0, *in1, *in2, *out, *_in0, *_in1, *vn;
    funcdata *fd = parent->fd;
    dobc *d = fd->d;
    int ret = 0, i;
    pcodeop *store, *op, *op1;
    flowblock *b, *bb;

    out = output;
    in0 = get_in(0);

    switch (opcode) {
    case CPUI_COPY:
        if (!is_trace() && !in0->is_input() && ((op = in0->def) && op->opcode == CPUI_COPY)) {
            _in0 = op->get_in(0);
            if ((_in0->get_addr() == out->get_addr()) && ((_in0->version + 1) == out->version)) {
                to_copy(_in0);

                if (in0->uses.size() == 0)
                    fd->op_destroy(op);
            }
        }

        if (in0->get_addr().getSpace() == d->ram_spc)
            loadram2out(Address(d->getDefaultCodeSpace(), in0->get_addr().getOffset()));
        else if (in0->is_constant()) {
            if ((in0->size == 16) && in0->get_val()) {
                //vm_error("pcode in size == 16, pcode(p%d)\n", start.getTime());
            }

            out->set_val(in0->get_val());
        }
        else if (fd->is_sp_constant(in0)) {
            out->set_sp_constant(in0->get_val());

            /* 识别这种特殊形式

            sp = ma + 4;

            ma = sp

            转换成
            ma = ma + 4;

            不处理load是怕会影响别名分析
            */
            op = in0->def;
            if (!is_trace()
                && (in0->uses.size() == 1)
                && !in0->flags.input
                && (op->opcode == CPUI_INT_ADD)) {
                _in0 = op->get_in(0);
                _in1 = op->get_in(1);

                /* 后面那个判断都是用来确认，活跃范围的 */
                //if (_in1->is_constant() && (_in0->get_addr() == output->get_addr()) && ((_in0->version + 1) == (output->version))) {
                if (_in1->is_constant() && _in0->in_liverange(this)) {
                    fd->op_remove_input(this, 0);
                    fd->op_set_opcode(this, op->opcode);

                    for (int i = 0; i < op->num_input(); i++) {
                        fd->op_set_input(this, op->get_in(i), i);
                    }

                    fd->op_destroy(op);
                }
            }
        }
        else
            out->type = in0->type;

        break;

        //      out                            in0              in1            
        // 66:(register,r1,4) = LOAD (const,0x11ed0f68,8) (const,0x840c,4)
    case CPUI_LOAD:
        fd->vmp360_marker(this);

        in1 = get_in(1);
        in2 = (inrefs.size() == 3) ? get_in(2):NULL;
        if (fd->is_code(in0, in1) && (in1->is_constant() || in1->is_pc_constant())) {
            loadram2out(Address(d->getDefaultCodeSpace(), in1->type.v));
            //printf("addr=%llx, pcode=%d, load ram, pos = %llx, val = %llx\n", get_dis_addr().getOffset(), start.getTime(), in1->type.v, out->get_val());
        }
        else if (in2 && (op = in2->def)) { // 别名分析过
            output->type = in2->type;

            /* 
            mem[x] = r0(0)

            r0(1) = mem[x]
            修改第2条指令会 cpy r0(1), r0(0)
            */
            if (op->opcode == CPUI_STORE) {
                varnode *_in2 = op->get_in(2);
                if (!is_trace() && 
                    (((_in2->get_addr() == out->get_addr()) && (_in2->version + 1) == (out->version))
                        || _in2->in_liverange_simple(this))) {
                    while (num_input())
                        fd->op_remove_input(this, 0);

                    fd->op_set_opcode(this, CPUI_COPY);
                    fd->op_set_input(this, _in2, 0);
                }
            }
        }
        else if ((inslot >= 0)) { // trace流中
            pcodeop *maystoer = NULL;
            if ((store = fd->trace_store_query(this))) {
                if (store->opcode == CPUI_INT_SUB)
                    out->set_val(0);
                else
                    out->type = store->get_in(2)->type;
            }
            else {
                /* 这里有点问题，实际上可能是a_bottom */
                out->type.height = a_top;
            }
        }
        /* 假如这个值确认来自于外部，不要跟新他 */
        else if (!flags.input && !flags.val_from_sp_alloc)
            out->type.height = a_top;

        break;

        //
    case CPUI_STORE:
        fd->vmp360_marker(this);

        in1 = get_in(1);
        in2 = get_in(2);
        if (output) {
            output->type = in2->type;
        }

        break;

    case CPUI_BRANCH:
        in0 = get_in(0);

        if (!flags.branch_call) {
            *branch = parent->get_out(0);
        }
        ret = ERR_MEET_CALC_BRANCH;
        break;

    case CPUI_CBRANCH:
        in1 = get_in(1);
        /* 
        两种情况
        1. 
        2. */
        if ((in1->is_constant())) {
            blockedge *edge; 
            
            edge = in1->get_val() ? parent->get_true_edge() : parent->get_false_edge();
            *branch = edge->point;
            ret = ERR_MEET_CALC_BRANCH;

            if (!is_trace() && !fd->in_cbrlist(this))
                fd->cbrlist.push_back(this);
        }
        else if ((op = in1->def) && (op->opcode == CPUI_BOOL_NEGATE)
            && (_in0 = op->get_in(0)) && _in0->def && (_in0->def->opcode == CPUI_BOOL_NEGATE)
            && (vn = _in0->def->get_in(0))->in_liverange(_in0->def, this)) {
            fd->op_remove_input(this, 1);
            fd->op_set_input(this, vn, 1);
        }
        break;

    case CPUI_BRANCHIND:
        if (in0->is_constant()) {
            Address addr(d->getDefaultCodeSpace(), in0->get_val());

            if (!(op = fd->find_op(addr))) {
                printf("we found a new address[%llx] to analaysis\n", addr.getOffset());
                fd->addrlist.push_back(addr);
            }
            else {
                *branch = op->parent;
                ret = ERR_MEET_CALC_BRANCH;
            }
        }
        break;

    case CPUI_INT_EQUAL:
        in1 = get_in(1);
        if (in0->is_constant() && in1->is_constant()) {
            output->set_val(in0->get_val() == in1->get_val());
        }
        /*
        
        a = phi(c2, c3)
        b = c1
        d = a - b
        e = INT_EQUAL d, 0
        假如c1 不等于 c2, c3
        那么 e 也是可以计算的。

        NOTE:这样写的pattern会不会太死了？改成递归收集a的所有常数定义，但是这样会不会效率太低了。
        */
        else if (in1->is_constant()  && (in1->get_val() == 0)
            && (op = in0->def) && (op->opcode == CPUI_INT_SUB)
            && (op->get_in(0)->is_constant() || op->get_in(1)->is_constant())) {
            _in0 = op->get_in(0);
            _in1 = op->get_in(1);

            varnode *check = _in0->is_constant() ? _in1 : _in0;
            varnode *uncheck = _in0->is_constant() ? _in0 : _in1;
            pcodeop *phi = check->def;
            int equal = 0, notequal = 0;  // 2:top, 1:equal, 0:notequal

            output->set_top();
            if (phi && (phi->opcode == CPUI_MULTIEQUAL)) {
                for (i = 0; i < phi->inrefs.size(); i++) {
                    vn = phi->get_in(i);
                    if (!vn->is_constant()) break;

                    if (vn->get_val() != uncheck->get_val()) notequal++;
                    else equal++;
                }

                if (notequal == phi->inrefs.size())     
                    output->set_val(0);
                else if (equal == phi->inrefs.size())   
                    output->set_val(1);
            }
        }
        else
            output->set_top();
        break;

    case CPUI_INT_NOTEQUAL:
        in1 = get_in(1);
        if (in0->is_constant() && in1->is_constant()) {
            output->set_val(in0->get_val() != in1->get_val());
        }
        else
            output->type.height = a_top;
        break;

    case CPUI_INT_SLESS:
        in1 = get_in(1);
        if (in0->is_constant() && in1->is_constant()) {
            output->set_val(in0->get_val() < in1->get_val());
        }
        else
            output->type.height = a_top;
        break;

    case CPUI_INT_SLESSEQUAL:
        in1 = get_in(1);
        if (in0->is_constant() && in1->is_constant()) {
            output->set_val(in0->get_val() <= in1->get_val());
        }
        else
            output->type.height = a_top;
        break;

    case CPUI_INT_LESS:
        in1 = get_in(1);
        if (in0->is_constant() && in1->is_constant()) {
            output->set_val((uint8)in0->get_val() < (uint8)in1->get_val());
        }
        else
            output->type.height = a_top;
        break;

    case CPUI_INT_LESSEQUAL:
        in1 = get_in(1);
        if (in0->is_constant() && in1->is_constant()) {
            output->set_val((uint8)in0->get_val() <= (uint8)in1->get_val());
        }
        else
            output->type.height = a_top;
        break;

    case CPUI_INT_ZEXT:
        if (in0->is_constant()) {
            if (in0->size < output->size) {
                output->set_val(zext(in0->get_val(), in0->size, out->size));
            }
            else if (in0->size > output->size) {
                throw LowlevelError("zext in size > ouput size");
            }
        }
        else
            output->type.height = a_top;
        break;

    case CPUI_INT_SEXT:
        if (in0->is_constant()) {
            if (in0->size < output->size) {
                output->set_val(sext(in0->get_val(), in0->size, out->size));
            }
            else if (in0->size > output->size) {
                throw LowlevelError("zext in size > ouput size");
            }
        }
        else
            output->type.height = a_top;
        break;

    case CPUI_INT_ADD:
        in1 = get_in(1);
        if (in0->is_constant() && in1->is_constant()) {
            if (in0->size == 1)
                out->set_val((int1)in0->type.v + (int1)in1->type.v);
            else if (in0->size == 2)
                out->set_val((int2)in0->type.v + (int2)in1->type.v);
            else if (in0->size == 4)
                out->set_val((int4)in0->type.v + (int4)in1->type.v);
            else 
                out->set_val(in0->type.v + in1->type.v);
        }
        else if ((in0->is_pc_constant() && in1->is_constant())
            || (in0->is_constant() && in1->is_pc_constant())) {
            out->set_pc_constant(in0->get_val() + in1->get_val());
        }
        else if (fd->is_sp_constant(in0) && in1->is_constant()) {
			compute_add_sub();
        }
        else if (in0->is_constant() && fd->is_sp_constant(in1)) {
            out->set_sp_constant(in0->type.v + in1->type.v);
        }
        else
            out->type.height = a_top;
        break;

    case CPUI_INT_SUB:
        in1 = get_in(1);
        op = in0->def;

        if (in0->is_constant() && in1->is_constant()) {
            if (in0->size == 1)
                out->set_val((int1)in0->type.v - (int1)in1->type.v);
            else if (in0->size == 2)
                out->set_val((int2)in0->type.v - (int2)in1->type.v);
            else if (in0->size == 4)
                out->set_val((int4)in0->type.v - (int4)in1->type.v);
            else 
                out->set_val(in0->type.v - in1->type.v);
        }
        /*      out                             0                   1       */
        /* 0:(register,mult_addr,4) = INT_SUB (register,sp,4) (const,0x4,4) */
        else if (fd->is_sp_constant(in0) && in1->is_constant()) {
			compute_add_sub();
        }
        else if (in0->is_pc_constant() && in1->is_constant()) {
            out->set_pc_constant(in0->get_val() - in1->get_val());
        }
        /*
        peephole:

        a = c1;
        if (x) a = c2;

        if (a == c1) {
        }
        else {
            if (a == c2) {
                // block1
            }
            else {
                // block 2
            }
        }
        实际上block2是死的，因为a只有2个定值
        */
        else if (in0->is_top() && in1->is_constant() 
            && (parent->in.size() == 1)
            && (op = in0->def) && (op->opcode == CPUI_MULTIEQUAL) && (op->inrefs.size() == 2) && op->all_inrefs_is_constant() 
            && (op1 = (b = parent->get_in(0))->get_cbranch_sub_from_cmp())
            && op1->get_in(1)->is_constant()
            && (op1->get_in(0) == in0)
            && b->is_eq_cbranch()) {
            intb imm = (op->get_in(0)->get_val() == op1->get_in(1)->get_val()) ? op->get_in(1)->get_val() : op->get_in(0)->get_val();
            out->set_sub_val(in0->size, imm, in1->get_val());
        }
        /*

        a = b + c
        d = a - 1
        e = d - c

        ==>

        e = b - 1

        */
        else if ((op = in0->def) && (op->opcode == CPUI_INT_SUB)
            && op->get_in(1)->is_constant()
            && (op->get_in(1)->get_val() == 1)
            && (op1 = op->get_in(0)->def)
            && (op1->opcode == CPUI_INT_ADD)
            && op1->get_in(1) == in1
            && (_in0 = op1->get_in(0))
            && _in0->in_liverange(this)) {
            while (inrefs.size() > 0)
                fd->op_remove_input(this, 0);

            fd->op_set_input(this, _in0, 0);
            fd->op_set_input(this, fd->create_constant_vn(1, output->size), 1);
        }
        else {
            out->type.height = a_top;
        }
        break;

    case CPUI_INT_SBORROW:
        in1 = get_in(1);
        /* 
        wiki: The overflow flag is thus set when the most significant bit (here considered the sign bit) is changed 
        by adding two numbers with the same sign (or subtracting two numbers with opposite signs).

        SBORROW一般是用来设置ov标记的，根据wiki上的说法，当加了2个一样的数，MSB发生了变化时，设置overflow flag
        */
        if (in0->is_constant() && in1->is_constant()) {
            int e = 1;

            if (in0->size == 4) {
                int l = (int)in0->get_val();
                int r = -(int)in1->get_val();
                int o;

                if (MSB4(l) != MSB4(r)) {
                    out->set_val(1);
                }
                else {
                    o = l + r;
                    if (MSB4(o) != MSB4(l)) {
                        e = 0;
                        out->set_val(1);
                    }
                }
            }
            else {
                throw LowlevelError("not support");
            }

            if (e) out->set_val(0);
        }
        break;

    case CPUI_INT_2COMP:
        if (in0->is_constant()) {
            out->set_val1(-in0->get_val());
        }
        else
            out->set_top();
        break;

    case CPUI_INT_LEFT:
        in1 = get_in(1);
        if (in0->is_constant() && in1->is_constant()) {
            out->set_val1((uintb)in0->get_val() << (uintb)in1->get_val());
        }
        else if (in1->is_constant() && (in1->get_val() == 0x1f)
            && (op = in0->def)
            && (op->opcode == CPUI_INT_MULT)
            && (op1 = op->get_in(0)->def)
            && (op1->opcode == CPUI_INT_SUB)
            && (op->get_in(1) == op1->get_in(0))
            && (op1->get_in(1)->is_constant())
            && (op1->get_in(1)->get_val() == 1)) {
            out->set_val(0);
        }
        else
            out->type.height = a_top;
        break;

    case CPUI_INT_RIGHT:
        in1 = get_in(1);
        if (in0->is_constant() && in1->is_constant()) {
            out->set_val1((uintb)in0->get_val() >> (uintb)in1->get_val());
        }
        else
            out->type.height = a_top;
        break;

    case CPUI_INT_NEGATE:
        if (in0->is_constant()) {
            intb v = in0->get_val();
            out->set_val(~v);
        }
        /* peephole 
        a = 0 - b
        c = ~a
        ==> 
        a = ~b + 1
        c = ~a
        ==>
        c = b - 1
        */
        else if ((op = in0->def) && (op->opcode == CPUI_INT_SUB) 
            && (_in0 = op->get_in(0))->is_constant()
            && (_in0->get_val() == 0)
            && (_in1 = op->get_in(1))->in_liverange(this)) {
            while (inrefs.size() > 0)
                fd->op_remove_input(this, 0);

            fd->op_set_opcode(this, CPUI_INT_SUB);
            fd->op_set_input(this, _in1, 0);
            fd->op_set_input(this, fd->create_constant_vn(1, output->size), 1);
        }
        else
            out->type.height = a_top;
        break;

    case CPUI_INT_XOR:
        in1 = get_in(1);
        if (in0->is_constant() && in1->is_constant()) {
            out->set_val(in0->get_val() ^ in1->get_val());
        }
        else
            out->type.height = a_top;
        break;

    case CPUI_INT_AND:
        in1 = get_in(1);
        uintb l, r;
        if (in0->is_constant() && in1->is_constant()) {
            out->set_val(in0->get_val() & in1->get_val());
        }
        /*
        识别以下pattern:

        t1 = r6 * r6
        r2 = t1 + r6 (r2一定为偶数)

        r2 = r2 & 1 一定为0
        */
        else if (in1->is_constant() 
            && (in1->get_val() == 1) 
            && in0->def 
            && (in0->def->opcode == CPUI_INT_ADD)
            && (op = in0->def->get_in(0)->def)
            && (op->opcode == CPUI_INT_MULT)
            && (op->get_in(0)->get_addr() == op->get_in(1)->get_addr())
            && (op->get_in(0)->get_addr() == in0->def->get_in(1)->get_addr())) {
            out->set_val(0);
        }
        /* 新增
        rn = (~(x * (x - 1))) & 1 == 0

        那么rn == 1
        */
        else if (in0->is_constant() && (in0->get_val() == 1)
            && in1->def 
            && (in1->def->opcode == CPUI_INT_NEGATE)
            && (op = in1->def->get_in(0)->def)
            && (op->opcode == CPUI_INT_MULT)
            && (op1 = op->get_in(0)->def)
            && (op1->opcode == CPUI_INT_SUB)
            && (op->get_in(1) == op1->get_in(0))
            && (op1->get_in(1)->is_constant())
            && (op1->get_in(1)->get_val() == 1)
            ) {
            out->set_val(1);
        }
        /* (sp - even) & 0xfffffffe == sp - even
        因为sp一定是偶数，减一个偶数，也一定还是偶数，所以他和 0xfffffffe 相与值不变
        */
        else if (in0->is_sp_constant() && in1->is_constant() 
            && ((l = (in0->get_val() & in1->get_val())) == (r = (in0->get_val() & (((uintb)1 << (in0->size * 8)) - 1))))) {
            out->set_sp_constant(in0->get_val() & in1->get_val());
        }
        else if (in0->is_pc_constant() && in1->is_constant()) {
            out->set_pc_constant(in0->get_val() & in1->get_val());
        }
        /* 相与时，任意一个数为0，则为0 */
        else if ((in0->is_constant() && (in0->get_val() == 0)) || (in1->is_constant() && (in1->get_val() == 0))) {
            out->set_val(0);
        }
        else if ((op = in0->def)
            && ((op->opcode == CPUI_INT_MULT) 
                || ((op->opcode == CPUI_INT_AND) && (_in1 = op->get_in(1)) && _in1->is_constant() && _in1->get_val() == 0xffffffffffffffff) && (op = op->get_in(0)->def))
            && (op1 = op->get_in(0)->def)
            && (op1->opcode == CPUI_INT_SUB)
            && (op1->get_in(0) == op->get_in(1))
            && (op1->get_in(1)->is_constant())
            && (op1->get_in(1)->get_val() == 1)) 
        {
            /* 
            rn = ((x * (x - 1)) & 1
            rn = 0;
            */
            if (in1->is_constant() && in1->get_val() == 1) {
                out->set_val(0);
            }
            /*
            rn = (x * (x - 1)) & ((x * (x - 1)) ^ 0xfffffffe)
            rn is 0
            */
            else if ((op = in1->def) && (op->opcode == CPUI_INT_XOR)
                && op->get_in(0)->is_constant() 
                && (VAL_MASK(op->get_in(0)->get_val(), op->get_in(0)->size) == 0xfffffffe)
                && (op->get_in(1) == in0)) {
                out->set_val(0);
            }
            else if ((op = in1->def) && (op->opcode == CPUI_INT_XOR)
                && op->get_in(1)->is_constant() 
                && (VAL_MASK(op->get_in(1)->get_val(), op->get_in(1)->size) == 0xfffffffe)
                && (op->get_in(0) == in0)) {
                out->set_val(0);
            }
            else {
                out->set_top();
            }
        }
        else
            out->set_top();
        break;

    case CPUI_INT_OR:
        in1 = get_in(1);
        if (in0->is_constant() && in1->is_constant()) {
            out->set_val(in0->get_val() | in1->get_val());
        }
        /* 
        y = ~(x * (x - 1)) | 0xfffffffe
        y == -1
        */
        else if (in0->is_constant() && (in0->get_val() == -2)
            && in1->def
            && (in1->def->opcode == CPUI_INT_NEGATE)
            && (op = in1->def->get_in(0)->def)
            && (op->opcode == CPUI_INT_MULT)) {
            if ((op1 = op->get_in(1)->def)
                && (op1->opcode == CPUI_INT_SUB)
                && op1->get_in(1)->is_val(1)
                && op1->get_in(0) == op->get_in(0)) {
                out->set_val(-1);
            }
            else if ((op1 = op->get_in(0)->def)
                && (op1->opcode == CPUI_INT_SUB)
                && op1->get_in(1)->is_val(1)
                && op1->get_in(0) == op->get_in(1)) {
                out->set_val(-1);
            }
            else
                out->set_top();
        }
        else
            out->set_top();
        break;

    case CPUI_INT_MULT:
        in1 = get_in(1);
        if (in0->is_constant() && in1->is_constant()) {
            out->set_val(in0->get_val() * in1->get_val());
        }
        else
            out->type.height = a_top;
        break;

    case CPUI_BOOL_NEGATE:
        if (in0->is_constant()) {
            out->set_val(in0->get_val() ? 0:1);
        }
        else
            out->type.height = a_top;
        break;

    case CPUI_BOOL_XOR:
        in1 = get_in(1);
        if (in0->is_constant() && in1->is_constant()) {
            out->set_val(in0->get_val() ^ in1->get_val());
        }
        else
            out->type.height = a_top;
        break;

    case CPUI_BOOL_AND:
        in1 = get_in(1);
        if (in0->is_constant() && in1->is_constant()) {
            out->set_val(in0->get_val() & in1->get_val());
        }
        else
            out->type.height = a_top;
        break;

    case CPUI_BOOL_OR:
        in1 = get_in(1);
        if (in0->is_constant() && in1->is_constant()) {
            out->set_val(in0->get_val() | in1->get_val());
        }
        else
            out->type.height = a_top;
        break;

    case CPUI_MULTIEQUAL:
        if (inslot >= 0) {
            output->type = get_in(inslot)->type;
        }
        else if (!flags.force_constant){
            /* 
            int x, y;
            if (!a) {
                y = 0;
            }

            if (a) {
                y = 0;
            }

            */
            if ((inrefs.size() == 2) && (in1 = get_in(1))->is_constant() 
                && in0->def && in0->def->opcode == CPUI_MULTIEQUAL
                && (_in1 = in0->def->get_in(1))
                && _in1->is_constant() && (in1->get_val() == _in1->get_val())
                && in1->def
                && (in1->def->parent->in.size() == 1) 
                && (b = in1->def->parent->get_in(0))
                && b->ops.size()
                && (b->last_op()->opcode == CPUI_CBRANCH)
                && (_in1->def->parent->in.size() == 1)
                && (bb = _in1->def->parent->get_in(0))
                && (bb->last_op()->opcode == CPUI_CBRANCH)
                && (vn = bb->last_op()->get_in(1))
                && vn->def
                && (vn->def->opcode == CPUI_BOOL_NEGATE)
                && (vn = vn->def->get_in(0))
                && (b->last_op()->get_in(1) == vn)) {
                output->set_val(in1->get_val());
            }
            else {
                on_MULTIEQUAL();
            }
        }
        break;

    case CPUI_SUBPIECE:
        in1 = get_in(1);
        if (in0->is_constant() && in1->is_constant()) {
            int v = in1->get_val();
            out->set_val((in0->get_val() >> (v * 8)) & calc_mask(out->size));
        }
        break;

    default:
        break;
    }

    /* 返回跳转地址 */
    if ((this == parent->last_op()) && (parent->out.size() == 1))
        *branch = parent->get_out(0);

    return ret;
}

void            pcodeop::to_constant(void)
{
    funcdata *fd = parent->fd;

    while (num_input() > 0)
        fd->op_remove_input(this, 0);

    fd->op_set_opcode(this, CPUI_COPY);
    fd->op_set_input(this, fd->create_constant_vn(output->get_val(), output->size), 0);
}

void            pcodeop::to_constant1(void)
{
    funcdata *fd = parent->fd;
    int i;
    varnode *vn, *out = output;
    pcodeop *op;

    /*
    非trace条件才能开启常量持久化
    */
    if (is_trace()) return;

    /* simd别转了 */
    if (dobc::singleton()->is_simd(get_addr()) && fd->flags.disable_simd_to_const) return;
    /* FIXME:
    这里不对simd的phi节点做转换是为了在代码生成时减少工作量，否则我们需要在某些cfg头部插入vmov_imm的指令
    
    是否考虑修复掉他？  */
    if ((opcode == CPUI_MULTIEQUAL) && fd->d->is_vreg(poa(this))) return;

    /* 
    1. 当有output节点，且output节点为常量时，运行进行常量持久化 
    2. opcode不能为copy，因为常量持久化就是把常量节点，改成copy操作 
    3. output节点为temp的不允许进行常量化，这个会破坏原始的inst结构，导致无法识别出原先的inst，在codegen时加大了难度
    4. opcode不能为store，因为store有副作用，它的use也不是特别好计算 
    */
    if (output && output->is_constant() 
        && !dobc::singleton()->is_temp(output->get_addr()) 
        && (opcode != CPUI_COPY) 
        && (opcode != CPUI_STORE)) {
        /* phi节点转成 copy指令时，需要记录下这个copy节点来自于phi节点，在删除phi节点时，假如有需要可以删除这个copy  */
        if (opcode == CPUI_MULTIEQUAL) flags.copy_from_phi = 1;
        while (num_input() > 0) 
            fd->op_remove_input(this, 0);

        fd->op_set_opcode(this, CPUI_COPY);
        fd->op_set_input(this, fd->create_constant_vn(out->get_val(), out->size), 0);
    }
    /* 
    1. phi节点的in节点不能这么处理
    2. call节点不能常量持久化in， 
    3. store节点的in节点不能转换为常量，这个是因为thumb不支持 store 一个 imm 到一个地址上，假如我们常量化会导致不好做代码生成

    */
    else if (!fd->flags.disable_inrefs_to_const && (opcode != CPUI_MULTIEQUAL) && (opcode != CPUI_STORE) && !is_call()) {
        for (i = 0; i < inrefs.size(); i++) {
            vn = get_in(i);

            /* 后面那个判断条件是防止冲入的*/
            if (vn->is_constant()) {
                if (!vn->in_constant_space()) {
                    fd->op_unset_input(this, i);
                    fd->op_set_input(this, fd->create_constant_vn(vn->get_val(), vn->size), i);
                }
            }
            /* 这里重点说下in的非constant转换规则 

            output = op(in0, in1, ..., inN) 
            上面的指令中，
            1. 假如某个inN的def是一个copy的操作
            2. 拷贝来自于另外一个cpu寄存器(rN)
            3. 当前的pcode在那个rN的活跃范围内
            我们直接修改上面的指令为 
            output = op(in0, in1, ..., rN)
            */
            else if ((op = vn->def) && (op->opcode == CPUI_COPY) && op->get_in(0)->in_liverange_simple(this)) {
                fd->op_unset_input(this, i);
                fd->op_set_input(this, op->get_in(0), i);
            }
        }
    }
}

void            pcodeop::to_rel_constant()
{
    funcdata *fd = parent->fd;
    if (opcode != CPUI_MULTIEQUAL)
        throw LowlevelError("to_rel_constant() only support MULTIEQUAL");

    varnode *in0 = get_in(0);

    while (num_input() > 1)
        fd->op_remove_input(this, 1);

    fd->op_set_opcode(this, CPUI_INT_ADD);
    intb sub = output->get_val() - in0->get_val();
    fd->op_set_input(this, fd->create_constant_vn(sub, output->size), 1);
}

void            pcodeop::to_copy(varnode *in)
{
    funcdata *fd = parent->fd;

    while (num_input() > 0)
        fd->op_remove_input(this, 0);

    fd->op_set_opcode(this, CPUI_COPY);
    fd->op_set_input(this, in, 0);
}

void            pcodeop::to_nop(void)
{
    funcdata *fd = parent->fd;

    while (num_input() > 0)
        fd->op_remove_input(this, 0);

    if (output)
        fd->destroy_varnode(output);

    fd->op_set_opcode(this, CPUI_COPY);
}
