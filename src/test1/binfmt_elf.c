
#include "binfmt_elf.h"
#include "mcore/mcore.h"

static int kernel_read(FILE *fp, long offset, void *buf, int bufsiz)
{
    fseek(fp, offset, SEEK_SET); 
    return fread(buf, bufsiz, 1, fp); 
}

static int load_elf_library(FILE *fp)
{
    Elf32_Phdr *elf_phdata = NULL;
    Elf32_Phdr *eppnt;
    int retval, error, i, j;
    Elf32_Ehdr elf_ex;

    error = -ENOEXEC;
    retval = kernel_read(fp, 0, (char *)&elf_ex, sizeof (elf_ex));
    if (retval != sizeof(elf_ex))
        goto out;

    if (memcmp(elf_ex.e_ident, ELFMAG, SELFMAG) != 0)
        goto out;

    j = sizeof(Elf32_Phdr) * elf_ex.e_phnum;

    error = -ENOMEM;
    elf_phdata = malloc(j);
    if (!elf_phdata)
        goto out;

    eppnt = elf_phdata;
    error = -ENOEXEC;
    retval = kernel_read(fp, elf_ex.e_phoff, (char *)eppnt, j);
    if (retval != j)
        goto out;

    for (j = 0, i = 0; i < elf_ex.e_phnum; i++) {
        if ((eppnt + i)->p_type == PT_LOAD)
            j++;
    }

out:
    if (elf_phdata)
        free(elf_phdata);

    return error;
}
