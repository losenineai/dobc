
#ifndef __codegen_h__
#define __codegen_h__

#include "dobc.hh"

class codegen {
public:
    funcdata *fd;


    codegen(funcdata *f) { fd = f;  }
    ~codegen();
};

class cgtrie_node {
public:
    pcodeop *p;
    vector<cgtrie_node *>   nodes;

    cgtrie_node();
};

class cgtrie {
public:
    cgtrie_node root;

    cgtrie() {}
};

#endif
