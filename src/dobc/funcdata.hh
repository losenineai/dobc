#ifndef __funcdata_h__
#define __funcdata_h__

#include <stdio.h>
#include "sleigh.hh"

class funcdata;
class varnode;
class pcodeop;

class pcodeemit2 : public PcodeEmit {
public:
    funcdata *fd = NULL;
    pcodeop *prevp = NULL;
    FILE *fp = stdout;
    int itblock = 0;
    virtual void dump(const Address &address, OpCode opc, VarnodeData *outvar, VarnodeData *vars, int size);

    void set_fp(FILE *f) { fp = f;  }
    void enter_itblock() { itblock = 1;  }
    void exit_itblock() { itblock = 0;  }
};

int print_vartype(Translate *trans, char *buf, varnode *data);

#define COLOR_ASM_INST_MNEM             "#3933ff"               
#define COLOR_ASM_INST_BODY             "#3933ff"               
#define COLOR_ASM_ADDR                  "#33A2FF"               
#define COLOR_ASM_STACK_DEPTH           "green"

class AssemblyRaw : public AssemblyEmit {

public:
    char *buf = NULL;
    FILE *fp = NULL;
    int sp = 0;
    int enable_html = 1;
    int mnem1 = 0;

    virtual void dump(const Address &addr, const string &mnem, const string &body);


    void set_mnem(int m) { mnem1 = m; }
    void set_buf(char *b) { buf = b; }
    void set_fp(FILE *f) { fp = f; }
    void set_sp(int s) { sp = s;  };
    void disable_html() { enable_html = 0;  }
};

#endif
