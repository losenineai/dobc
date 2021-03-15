
#include "dobc.hh"


typedef list<pcodeop *>::iterator pit;

struct thumb_gen {
    funcdata *fd;
    dobc *d;
    vector<flowblock *> blist;

    char *buf;
    int size;

    thumb_gen(funcdata *f);
    ~thumb_gen();

    void resort_blocks();
    int run();

    int reg2index(const Address &a);

    int _push(int32_t reglist, char *buf, int size);

    pit g_push(flowblock *b, pit pit);
    pit g_sub(flowblock *b, pit pit);
};