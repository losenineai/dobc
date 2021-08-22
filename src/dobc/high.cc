
#include "dobc.hh"
#include "high.hh"

#define clist_add(p1, p2, n) do { \
        p1->lnode.not = n; \
        high_cond *tmp = p1->lnode.next; \
        p1->lnode.next = (p2); \
        (p2)->lnode.next = tmp; \
        (p2)->lnode.prev = p1; \
        if (tmp) \
            tmp->lnode.prev = (p2); \
    } while (0)

high_cond::high_cond()
{
}

high_cond::~high_cond()
{
}

#define in0(_p)                (_p)->get_in(0)
#define in1(_p)                (_p)->get_in(1)
#define in0a(_p)                in0(_p)->get_addr()
#define in1a(_p)                in1(_p)->get_addr()

int high_cond::compute_cond(flowblock *b)
{
    dobc *d = dobc::singleton();
    vector<pcodeop *> s;
    list<pcodeop *>::reverse_iterator rit = b->ops.rbegin();
    const Address &last_addr = (*rit)->get_addr();

    lnode.next = lnode.prev = NULL;
    lnode.not = 0;

    for (++rit; rit != b->ops.rend(); rit++) {
        if ((*rit)->get_addr() == last_addr)
            s.push_back(*rit);
        else
            break;
    }

    version = b->fd->reset_version;

    if (detect_operands(b->last_op()))
        return -1;

    switch (s[0]->opcode) {
    case CPUI_BOOL_NEGATE:
        switch (s[1]->opcode) {
        case CPUI_INT_EQUAL:
            if (in0a(s[1]) == d->zr_addr) {

                if (in1(s[1])->is_val(0)) {
                    type = h_eq;
                    return 0;
                }

                if (in1(s[1])->is_val(1)) {
                    type =  h_ne;
                    return 0;
                }
            }
            else if ((in0a(s[1]) == d->ng_addr) && (in1a(s[1]) == d->ov_addr)) {
                type = h_lt;
                return 0;
            }
            break;

        case CPUI_INT_NOTEQUAL:
            if ((in0a(s[1]) == d->ng_addr) && (in1a(s[1]) == d->ov_addr)) {
                type = h_ge;
                return 0;
            }
            break;
        }

        break;
    }

    return -1;
}

int high_cond::update(flowblock *t)
{
    from = t;

    if (version != t->fd->reset_version)
        return compute_cond(t);

    return 0;
}

int high_cond::update(flowblock *from, flowblock *to1)
{
    from->update_cond();

    if ((from->get_true_block() == to1)) {
        *this = from->cond;
    }
    else {
        *this = not(from->cond);
    }

    to = to1;

    return 0;
}

enum high_cond_type  high_cond::get_type(void)
{
    funcdata *fd = from->fd;

    if (version != fd->reset_version)
        update(from);

    return type;
}

high_cond &high_cond::not(const high_cond &op2)
{
    *this = op2;

    type = nottype(op2.type);

    return *this;
}

high_cond_type high_cond::nottype(high_cond_type t) const
{
    high_cond_type ret = h_unkown;
    switch (t) {
    case h_eq: ret = h_ne; break;
    case h_ne: ret = h_eq; break;
    case h_cs: ret = h_cc; break;
    case h_cc: ret = h_cs; break;
    case h_lt: ret = h_ge; break;
    case h_ge: ret = h_lt; break;
    case h_le: ret = h_gt; break;
    case h_gt: ret = h_le; break;
    default:
        throw LowlevelError("high_cond not support type");
        break;
    }

    return ret;
}

int     high_cond::linkto(high_cond &op2)
{
    pcodeop *p = op2.lhs->def;
    funcdata *fd = from->fd;
    flowblock *middle;

    if ((p->opcode != CPUI_MULTIEQUAL)) {
        /*
        判断2个比较的结构是否相等 
        */
        if (op2.lhs == lhs 
            && ((op2.rhs == rhs) 
                || (op2.rhs->is_constant() && rhs->is_constant() && op2.rhs->get_val() == rhs->get_val()))) {
            if (type == op2.type) {
                clist_add(this, &op2, 0);
            }
            else if (type == nottype(op2.type)) {
                clist_add(this, &op2, 1);
            }

            return 0;
        }

        if (!p->all_inrefs_is_constant())
            return -1;

    }

    if (!fd->is_ifthenfi_structure(from, middle = op2.from->get_max_dfnum_in(), op2.from))
        return -1;

    varnode *truevn = from->get_true_vn(p);

    int link_not = 0;
    if (op2.type == h_eq) {

        if (!op2.rhs->is_val(truevn->get_val()))
            link_not = 1;

        clist_add(this, &op2, link_not);

        return 0;
    }

    return -1;
}

int     high_cond::process_zr(varnode *zr)
{
    pcodeop *p = zr->def, *p1;

    switch (p->opcode) {
    case CPUI_INT_EQUAL:
        p1 = in0(p)->def;
        if (in1(p)->is_constant() && in1(p)->get_val() == 0) {
            if ((p1->opcode == CPUI_INT_XOR) && in1(p1)->is_constant() && (in1(p1)->get_val() == 1)) {
                lhs = in0(p1);
                rhs = in1(p1);
                return 0;
            }
        }
        break;
    }

    return -1;
}

int high_cond::detect_operands(pcodeop *op)
{
    vector<pcodeop *> ops;

    ops.push_back(op);
    while (!ops.empty()) {
        op = *(ops.erase(ops.begin()));

        for (int i = 0; i < op->inrefs.size(); i++) {
            varnode *vn = op->get_in(i);

            if (dobc::singleton()->is_greg(vn->get_addr())) {
                if (op->inrefs.size() == 2) {
                    lhs = op->get_in(0);
                    rhs = op->get_in(1);
                    return 0;
                }
                else
                    throw LowlevelError("only support 2 operands opcode");
            }
            else if (vn->def)
                ops.push_back(vn->def);
        }
    }

    return -1;
}

high_cond_table::high_cond_table(funcdata *fd1)
{
    fd = fd1;
}

high_cond_table::~high_cond_table()
{
}

int high_cond_table::clear()
{
    high_cond_set::iterator it = tabs.begin();

    for (; it != tabs.end(); it++) {
    }

    return 0;
}

bool high_cond_cmp_def::operator()(const high_cond *a, const high_cond *b) const
{
    return a->from->dfnum < b->from->dfnum;
}

high_cond *high_cond::get_cond_link_head(int &not)
{
    high_cond *n = this;
    not = 0;

    while (n->lnode.prev) {
        n = n->lnode.prev;
    }

    return n;
}

