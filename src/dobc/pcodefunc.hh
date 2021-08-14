

#ifndef __pcodefunc_h__
#define __pcodefunc_h__

class funcdata;
class dobc;
class flowblock;
class varnode;
class pcodeop;

class pcodefunc {
public:
    funcdata *fd;
    dobc *d;
    pcodefunc(funcdata *f);
    void add_cmp_const(flowblock *b, list<pcodeop *>::iterator it, const varnode *rn, const varnode *v);
    void add__cbranch_eq(flowblock *b, int eq);
    void add_cbranch_eq(flowblock *b);
    void add_cbranch_ne(flowblock *b);
    void add_copy_const(flowblock *b, list<pcodeop *>::iterator it, const varnode &rd, const varnode &v);
};

#endif