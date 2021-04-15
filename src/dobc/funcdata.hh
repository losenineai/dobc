#ifndef __funcdata_h__
#define __funcdata_h__

#include <stdio.h>
#include "sleigh.hh"

class funcdata;
class varnode;

class pcodeemit2 : public PcodeEmit {
public:
    funcdata *fd = NULL;
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

    virtual void dump(const Address &addr, const string &mnem, const string &body) {
        if (buf) {
            if (enable_html)
                sprintf(buf, "<tr>"
                    "<td><font color=\"" COLOR_ASM_STACK_DEPTH "\">%03x:</font></td>"
                    "<td><font color=\"" COLOR_ASM_ADDR "\">0x%04x:</font></td>"
                    "<td align=\"left\"><font color=\"" COLOR_ASM_INST_MNEM "\">%s </font></td>"
                    "<td align=\"left\"><font color=\"" COLOR_ASM_INST_BODY "\">%s</font></td></tr>",
                    sp, (int)addr.getOffset(), mnem.c_str(), body.c_str());
            else
                sprintf(buf, "0x%04x: %s %s", (int)addr.getOffset(), mnem.c_str(), body.c_str());
            //sprintf(buf, "0x%08x:%10s %s", (int)addr.getOffset(), mnem.c_str(), body.c_str());
        }
        else {
            if (fp)
                fprintf(fp, "0x%04x: %s %s\n", (int)addr.getOffset(), mnem.c_str(), body.c_str());
            else
                fprintf(stdout, "0x%04x: %s %s\n", (int)addr.getOffset(), mnem.c_str(), body.c_str());
        }
    }

    void set_buf(char *b) { buf = b; }
    void set_fp(FILE *f) { fp = f; }
    void set_sp(int s) { sp = s;  };
    void disbable_html() { enable_html = 0;  }
};

#endif
