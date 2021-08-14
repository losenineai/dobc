
#include "dobc.hh"
#include "high.hh"

high_cond::high_cond()
{
}

high_cond::~high_cond()
{
}

int high_cond::update(flowblock *t)
{
    return 0;
}

int high_cond::update(flowblock *from, flowblock *to)
{
    update(from);
    this->to = to;

    if ((from->get_true_block() == to)) {
        *this = from->hi_cond;
    }
    else {
        *this = not(from->hi_cond);
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
    default:
        throw LowlevelError("high_cond not support type");
        break;
    }

    return *this;
}