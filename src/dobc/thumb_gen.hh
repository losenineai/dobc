
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


struct thumb_gen {
    funcdata *fd;
    dobc *d;
    vector<flowblock *> blist;

    char *asmbuf;
    int asmlen;
    int asmsize;

    thumb_gen(funcdata *f);
    ~thumb_gen();

    void resort_blocks();
    int run();

    int reg2index(const Address &a);

    pit g_push(flowblock *b, pit pit);

    pit g_add(flowblock *b, pit pit);
    pit g_sub(flowblock *b, pit pit);

    pit g_ldr(flowblock *b, pit pit);
    pit g_str(flowblock *b, pit pit);
    pit g_mov(flowblock *b, pit pit);
    pit g_blx(flowblock *b, pit pit);
};