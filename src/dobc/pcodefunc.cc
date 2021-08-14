
#include "vm.h"
#include "sleigh.hh"
#include "pcodefunc.hh"
#include "dobc.hh"

pcodefunc::pcodefunc(funcdata *f)
{
    fd = f;
    d = fd->d;
}

void pcodefunc::add_cmp_const(flowblock *b, list<pcodeop *>::iterator it, const varnode *rn, const varnode *v)
{
    pcodeop *op[3];
    int i;

    i = 0;
    if (rn->is_sp_vn()) {
        op[i] = fd->newop(1, SeqNum(Address(d->trans->getDefaultCodeSpace(), fd->user_offset), fd->op_uniqid++));
        op[i]->set_opcode(CPUI_USE);
        fd->op_set_input(op[i], fd->create_vn(rn->get_size(), d->sp_addr), 0);
        i++;
    }

    op[i] = fd->newop(2, SeqNum(Address(d->trans->getDefaultCodeSpace(), fd->user_offset), fd->op_uniqid++));
    op[i]->set_opcode(CPUI_INT_SUB);
    fd->op_set_input(op[i], fd->clone_varnode(rn), 0);
    fd->op_set_input(op[i], fd->create_constant_vn(v->get_val(), v->get_size()), 1);
    fd->new_unique_out(rn->get_size(), op[i]);
    i++;

    op[i] = fd->newop(2, SeqNum(Address(d->trans->getDefaultCodeSpace(), fd->user_offset), fd->op_uniqid++));
    op[i]->set_opcode(CPUI_INT_EQUAL);
    fd->op_set_input(op[i], op[i-1]->output, 0);
    fd->op_set_input(op[i], fd->create_constant_vn(0, v->get_size()), 1);
    fd->new_varnode_out(1, d->zr_addr, op[i]);
    i++;

    fd->user_offset++;
    for (int j = 0; j < i; j++) {
         fd->op_insert(op[j], b, b->ops.end());
    }
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

