
#include "dobc.hh"
#include "varnode.hh"

varnode::varnode(funcdata *f, int s, const Address &m)
    : loc(m)
{
    if (!m.getSpace())
        return;

    fd = f;
    size = s;

    spacetype tp = m.getSpace()->getType();

    type.height = a_top;

    if (tp == IPTR_CONSTANT) {
        nzm = m.getOffset();

        set_val(m.getOffset());
    }
    else if ((tp == IPTR_FSPEC) || (tp == IPTR_IOP)) {
        flags.annotation = 1;
        flags.covertdirty = 1;
        nzm = ~((uintb)0);
    }
    else {
        flags.covertdirty = 1;
        nzm = ~((uintb)0);
    }

}

varnode::~varnode()
{
}

bool            varnode::in_ram() 
{ 
    return get_addr().getSpace() == dobc::singleton()->ram_spc; 
}

void            varnode::set_def(pcodeop *op)
{
    def = op;
    if (op) {
        flags.written = 1;
    }
    else
        flags.written = 0;
}

void            varnode::add_use(pcodeop *op)
{
    uses.push_back(op);
}

void            varnode::del_use(pcodeop *op)
{
    list<pcodeop *>::iterator iter;

    iter = uses.begin();
    while (*iter != op)
        iter ++;

    uses.erase(iter);
    flags.covertdirty = 1;
}

pcodeop*        varnode::lone_use()
{
    return (uses.size() == 1) ? *(uses.begin()) : NULL;
}

bool            varnode::in_liverange(pcodeop *p)
{
	return in_liverange_simple(p);
}

bool			varnode::in_liverange_simple(pcodeop *p)
{
	int v = p->parent->fd->reset_version;

	if (v != simple_cover.version) return false;
	if (p->parent->index != simple_cover.blk_index) return false;

	int o = p->start.getOrder();
	if ((o >= simple_cover.start) && (simple_cover.start > -1)) {
		if ((o <= simple_cover.end) || (simple_cover.end == -1))
			return true;
	}

	return false;
}

bool            varnode::in_liverange(pcodeop *start, pcodeop *end)
{
    if (start->parent != end->parent)
        return false;

    list<pcodeop *>::iterator it = start->basiciter;

    for (; it != end->basiciter; it++) {
        pcodeop *p = *it;

        if (p->output && p->output->get_addr() == get_addr())
            return false;
    }

    return true;
}

void			varnode::add_def_point_simple()
{
	simple_cover.version = def->parent->fd->reset_version;
	simple_cover.blk_index = def->parent->index;
	simple_cover.start = def->start.getOrder() + 1;
	simple_cover.end = -1;
}

void			varnode::add_ref_point_simple(pcodeop *p)
{
	short v = p->parent->fd->reset_version;

	if (simple_cover.version != v) {
		if (is_input()) {
			simple_cover.blk_index = 0;
			simple_cover.version = v;
			simple_cover.start = 0;
			simple_cover.end = p->start.getOrder();
		}
		else
			throw LowlevelError("un-input varnode version mismatch");
	}
	else {
		/* 假如不是同一个块，则返回 */
		if (p->parent->index != simple_cover.blk_index)
			return;

		/* 假如是同一个块，则设置为终点 */
		simple_cover.end = p->start.getOrder();
	}
}

void			varnode::clear_cover_simple()
{
	simple_cover.version = -1;
	simple_cover.blk_index = -1;
	simple_cover.start = -1;
	simple_cover.end = -1;
}

pcodeop*        varnode::search_copy_chain(OpCode until, flowblock *until_blk)
{
    pcodeop *p = def;
    varnode *vn = NULL;

    while (p && (p->opcode != until)) {

        vn = NULL;
        switch (p->opcode) {
        case CPUI_STORE:
        case CPUI_LOAD:
            vn = p->get_virtualnode();
            break;

        case CPUI_SUBPIECE:
        case CPUI_COPY:
            vn = p->get_in(0);
            break;

        }

        if (!vn || !vn->def) return p;
        if (until_blk && (vn->def->parent != until_blk)) return p;

        p = vn->def;
    }

    return (p && (p->opcode == until)) ? p : NULL;
}

bool            varnode::maystore_from_this(pcodeop *p)
{
    varnode *in0;

    while (p) {
        in0 = NULL;
        switch (p->opcode) {
        case CPUI_STORE:
            in0 = p->get_in(1);
            break;

        case CPUI_INT_ADD:
            if (p->get_in(1)->is_constant() && p->get_in(1)->get_val() == 0) 
                in0 = p->get_in(0);
            break;
        }

        if (in0 && in0 == this)
            return true;

        p = (in0 && in0->def) ? in0->def : NULL;
    }

    return false;
}

bool            varnode::is_sp_vn() const
{ 
    return (dobc::singleton()->getStackBaseSpace()) == get_addr().getSpace();  
}

intb            varnode::get_val(void) const
{
    return type.v;
}

void            varnode::collect_all_const_defs()
{
    int dfnum;

#if 0
    if (fd->heritage_times == heritage_ver)
        return;
#endif

    const_defs.clear();
    top_defs.clear();

    if (is_gen_constant())
        const_defs.push_back(this);

    fd->collect_all_const_defs(def, const_defs, top_defs, dfnum, 1);

    heritage_ver = fd->heritage_times;
}

