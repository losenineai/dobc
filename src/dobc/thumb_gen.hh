
#ifndef __thumb_gen_h__
#define __thumb_gen_h__

#include "dobc.hh"

typedef list<pcodeop *>::iterator pit;

#define R0          0
#define R1          1
#define R2          2
#define R3          3
#define R4          4
#define R5          5
#define R6          6
#define R7          7
#define R8          8
#define R9          9
#define R10         10 
#define R11         11
#define R12         12
#define SP          13
#define LR          14
#define PC          15

struct fix_item {
    int ind;
    flowblock *b;

    fix_item(int i, flowblock *b1) { ind = i; b1 = b;  }
};

struct thumb_gen {
    funcdata *fd;
    dobc *d;
    vector<flowblock *> blist;
    vector<fix_item *> flist;
    Address addr;

    unsigned char *data;
    int ind;

    thumb_gen(funcdata *f);
    ~thumb_gen();

    void resort_blocks();
    int run();
    int run_block(flowblock *b);
    void add_fix_list(int ind, flowblock *b);

    uint32_t reg2index(const Address &a);
    int     calc_code_size(flowblock *b);

    pit g_push(flowblock *b, pit pit);
    pit g_add(flowblock *b, pit pit);

    void dump();
};

#endif