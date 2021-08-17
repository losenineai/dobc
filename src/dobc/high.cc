
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

enum high_cond_type high_cond::compute_cond(flowblock *b)
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

    switch (s[0]->opcode) {
    case CPUI_BOOL_NEGATE:
        switch (s[1]->opcode) {
        case CPUI_INT_EQUAL:
            if (in0a(s[1]) == d->zr_addr) {
                if (in0(s[1])->is_val(0))
                    return h_eq;

                if (in0(s[1])->is_val(1))
                    return h_ne;
            }
            break;

        case CPUI_INT_NOTEQUAL:
            if ((in0a(s[1]) == d->ng_addr) && (in0a(s[1]) == d->ov_addr))
                return h_ge;
            break;
        }

        break;
    }

    return h_unkown;
}

int high_cond::update(flowblock *t)
{
    type = compute_cond(t);

    return 0;
}

int high_cond::update(flowblock *from, flowblock *to)
{
    if (update(from) || (type == h_unkown))
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