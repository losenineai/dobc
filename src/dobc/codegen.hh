
#ifndef __codegen_h__
#define __codegen_h__

#include "dobc.hh"

typedef map<intb, pcodeop *>         pc_rel_table;

class codegen {
public:
    funcdata *fd;

    /* 《Linear Scan Register Allocation for the Java HotSpot™ Client Compiler》 
    */
    void sort_blocks(vector<flowblock *> &blks);
    codegen(funcdata *f) { fd = f;  }
    ~codegen() {}
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
