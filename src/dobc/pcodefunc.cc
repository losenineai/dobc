
#include "sleigh.hh"
#include "pcodefunc.hh"

#define count_of_array(a)       (sizeof(a) / sizeof (a[0]))

pcodefunc::pcodefunc(funcdata *f)
{
    fd = f;
    d = fd->d;
}

void pcodefunc::add_cmp_const(flowblock *b, varnode *rn, varnode *v)
{
    pcodeop *op[2];

    int i;

    for (i = 0; i < count_of_array(op); i++) {
        op[i] = fd->newop(2, SeqNum(Address(d->trans->getDefaultCodeSpace(), fd->user_offset++), fd->op_uniqid++));
        fd->op_insert_end(op[i], b);
    }
    op[0]->set_opcode(CPUI_INT_SUB);
    op[1]->set_opcode(CPUI_INT_EQUAL);

}

void pcodefunc::add_cbranch_eq(flowblock *b)
{
}

void pcodefunc::add_cbranch_ne(flowblock *b)
{
}
