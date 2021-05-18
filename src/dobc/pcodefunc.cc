
#include "sleigh.hh"
#include "pcodefunc.hh"

pcodefunc::pcodefunc(funcdata *f)
{
    fd = f;
    d = fd->d;
}

void pcodefunc::add_cmp_const(flowblock *b, list<pcodeop *>::iterator it, const varnode *rn, const varnode *v)
{
    pcodeop *op[2];
    int i, end = 0;

    if (it == b->ops.end()) end = 1;

    for (i = 0; i < count_of_array(op); i++) {
        op[i] = fd->newop(2, SeqNum(Address(d->trans->getDefaultCodeSpace(), fd->user_offset), fd->op_uniqid++));
        if (end) fd->op_insert(op[i], b, b->ops.end());
        else fd->op_insert(op[i], b, it++);
    }
    fd->user_offset++;

    op[0]->set_opcode(CPUI_INT_SUB);
    fd->op_set_input(op[0], fd->clone_varnode(rn), 0);
    fd->op_set_input(op[0], fd->create_constant_vn(v->get_val(), v->get_size()), 1);
    fd->new_unique_out(rn->get_size(), op[0]);

    op[1]->set_opcode(CPUI_INT_EQUAL);
    fd->op_set_input(op[1], op[0]->output, 0);
    fd->op_set_input(op[1], fd->create_constant_vn(0, v->get_size()), 1);
    fd->new_varnode_out(1, d->zr_addr, op[1]);
}

void pcodefunc::add__cbranch_eq(flowblock *b, int eq)
{
    pcodeop *op[2];
    int i;

    for (i = 0; i < count_of_array(op); i++) {
        op[i] = fd->newop(2, SeqNum(Address(d->trans->getDefaultCodeSpace(), fd->user_offset), fd->op_uniqid++));
        fd->op_insert_end(op[i], b);
    }
    fd->user_offset++;

    op[0]->set_opcode(CPUI_INT_EQUAL);
    fd->op_set_input(op[0], fd->new_varnode(1, d->zr_addr), 0);
    fd->op_set_input(op[0], fd->create_constant_vn(eq, 1), 1);
    fd->new_unique_out(1, op[0]);
    op[1]->set_opcode(CPUI_CBRANCH);
    fd->op_set_input(op[1], fd->new_unique(4), 0);
    fd->op_set_input(op[1], op[0]->output, 1);
}

void pcodefunc::add_cbranch_eq(flowblock *b)
{
    add__cbranch_eq(b, 1);
}

void pcodefunc::add_cbranch_ne(flowblock *b)
{
    add__cbranch_eq(b, 0);
}

void pcodefunc::add_copy_const(flowblock *b, list<pcodeop *>::iterator it, const varnode &rd, const varnode &v)
{
    pcodeop *op;
    op = fd->newop(1, SeqNum(Address(d->trans->getDefaultCodeSpace(), fd->user_offset++), fd->op_uniqid++));
    fd->op_insert(op, b, it);

    op->set_opcode(CPUI_COPY);
    fd->op_set_input(op, fd->create_constant_vn(v.get_val(), v.get_size()), 0);
    fd->op_set_output(op, fd->clone_varnode(&rd));
}

