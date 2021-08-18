
#include "dobc.hh"
#include "high.hh"

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

    for (++rit; rit != b->ops.rend(); rit++) {
        if ((*rit)->get_addr() == last_addr)
            s.push_back(*rit);
        else
            break;
    }

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
            break;

        case CPUI_INT_NOTEQUAL:
            if ((in0a(s[1]) == d->ng_addr) && (in0a(s[1]) == d->ov_addr)) {
                type = h_ge;
                return h_ge;
            }
            break;
        }

        break;
    }

    return -1;
}

int high_cond::update(flowblock *t)
{
    return compute_cond(t);
}

int high_cond::update(flowblock *from, flowblock *to)
{
    if (update(from))
        return -1;

    this->to = to;

    if ((from->get_true_block() == to)) {
        *this = from->cond;
    }
    else {
        *this = not(from->cond);
    }

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
    switch (op2.type) {
    case h_eq: type = h_ne; break;
    case h_ne: type = h_eq; break;
    case h_cs: type = h_cc; break;
    case h_cc: type = h_cs; break;
    case h_lt: type = h_ge; break;
    case h_ge: type = h_lt; break;
    case h_le: type = h_gt; break;
    case h_gt: type = h_le; break;
    default:
        throw LowlevelError("high_cond not support type");
        break;
    }

    return *this;
}

int     high_cond::linkto(high_cond &op2)
{
    link = &op2;

    return 0;
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