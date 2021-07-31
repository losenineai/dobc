
#ifndef __binfmt_elf_h__
#define __binfmt_elf_h__

#include "unicorn/unicorn.h"

#define BINPRM_BUF_SIZE     128

struct linux_binprm {
    char buf[BINPRM_BUF_SIZE];

    FILE *file;

    const char*     filename;
    const char*     interp;
    unsigned long   loader;
    unsigned long   exec;
};

struct linux_binfmt {
    int (* load_shlib)(struct FILE *);
    int (* core_dump)(void);
    unsigned long min_coredump;
};

#endif