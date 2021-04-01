
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
    int from;
    int cond;
    flowblock *to_blk;

    fix_item(int from1, flowblock *b1, int c) { from = from1; to_blk = b1; cond = c; }
};

struct thumb_gen {
    funcdata *fd;
    dobc *d;
    vector<flowblock *> blist;
    vector<fix_item *> flist;

    unsigned char *data;
    int ind;

    thumb_gen(funcdata *f);
    ~thumb_gen();

    void resort_blocks();
    int run();
    void save(void);
    int run_block(flowblock *b, int b_ind);
    void add_fix_list(int ind, flowblock *b, int op);

    uint32_t reg2index(const Address &a);
    int     calc_code_size(flowblock *b);

    pit g_push(flowblock *b, pit pit);
    pit g_pop(flowblock *b, pit pit);

    void dump();
    int dump_one_inst(int index, pcodeop *p);
};

#endif