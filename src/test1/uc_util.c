

#include "uc_util.h"
#include "mcore/mcore.h"
#include <assert.h>

#define DEFAULT_STACK_SIZE      (64 * 1024)
#define DEFAULT_HEAP_SIZE       (1024 * 1024)

#define DEFAULT_STACK_ADDR      (0x600000)
#define DEFAULT_HEAP_ADDR       (0x700000)

#define SO_ADDR                 0x10000

void                    ur_elf_lib_init(uc_runtime_t *t);

uc_runtime_t*   uc_runtime_new(uc_engine *uc, const char *soname, int stack_size, int heap_size)
{
    uc_runtime_t *ur;
    uc_err err;

    ur = calloc(1, sizeof (ur[0]) + strlen(soname));
    if (!ur)
        return NULL;

    ur->fdata = file_load(soname, &ur->flen);
    if (!ur->fdata) {
        print_err ("err: %s failed when file_load. %s:%d\r\n",  __func__, __FILE__, __LINE__);
        return NULL;
    }

    ur->uc = uc;
    strcpy(ur->soname, soname);

    if (!stack_size)
        ur->stack.size = DEFAULT_STACK_SIZE;

    if (!heap_size)
        ur->heap.size = DEFAULT_HEAP_SIZE;

    ur->stack.data = DEFAULT_STACK_ADDR;
    ur->heap.data = DEFAULT_HEAP_ADDR;

    ur->text.data = SO_ADDR;
    ur->text.size = MB;

    ur->rel.data = 0x800000;
    ur->rel.size = 0x1000;

    if ((err = uc_mem_map(uc, ur->stack.data, ur->stack.size, UC_PROT_READ | UC_PROT_WRITE))
        || (err = uc_mem_map(uc, ur->heap.data, ur->heap.size, UC_PROT_READ | UC_PROT_WRITE))
        || (err = uc_mem_map(uc, ur->text.data, ur->text.size, UC_PROT_ALL))
        || (err = uc_mem_map(uc, ur->rel.data, ur->rel.size, UC_PROT_ALL))) {
        print_err ("err: %s  failed with (%d:%s)uc_mem_map. %s:%d\r\n",  __func__, err, uc_strerror(err), __FILE__, __LINE__);
        return NULL;
    }

    ur->heap.end = ur->heap.data + ur->heap.size - 1;
    ur->stack.end = ur->stack.data + ur->stack.size - 1;
    ur->text.end = ur->text.data + ur->text.size - 1;

    uc_mem_write(uc, SO_ADDR, ur->fdata, ur->flen);

    ur_elf_lib_init(ur);

    return ur;
}

uc_pos_t            ur__malloc(uc_runtime_t *r, struct uc_area *area, int size)
{
    int ind = area->index;

    if ((ind + size) > area->size)
        return 0;

    area->index += size;

    return area->data + ind;
}

uc_pos_t            ur_malloc(uc_runtime_t *r, int size)
{
    return ur__malloc(r, &r->heap, size);
}

uc_pos_t           ur_calloc(uc_runtime_t *uc, int elmnum, int elmsiz)
{
    return 0;
}

uc_pos_t            ur_text_start(uc_runtime_t *r)
{
    return r->text.data;
}

uc_pos_t        ur_text_end(uc_runtime_t *r)
{
    return r->text.end;
}

void            ur_free(void* v)
{
}

uc_pos_t        ur_strcpy(uc_runtime_t *r, uc_pos_t dst, uc_pos_t src)
{
    assert(0);
    return 0;
}


uc_pos_t        ur_stack_start(uc_runtime_t *r)
{
    return r->stack.data;
}

uc_pos_t        ur_stack_end(uc_runtime_t *r)
{
    return r->stack.end;
}

uc_pos_t        ur_symbol_addr(uc_runtime_t *r, const char *symname)
{
    Elf32_Sym *sym = elf32_sym_get_by_name((Elf32_Ehdr *)r->fdata, symname);

    return r->text.data + sym->st_value;
}

void*           uc_vir2phy(uc_pos_t t)
{
    return 0;
}

uc_pos_t        ur_string(uc_runtime_t *r, const char *str)
{
    int len = strlen(str);
    uc_pos_t p = ur_malloc(r, len + 1);

    if (!p)
        return 0;

    uc_mem_write(r->uc, p, str, len + 1);

    return p;
}

struct uc_hook_func*    ur_hook_func_find(uc_runtime_t *r, const char *name)
{
    int i;
    struct uc_hook_func *f;

    for (i = 0, f= r->hooktab.list; i < r->hooktab.counts; i++, f = f->node.next) {
        if (!strcmp(f->name, name))
            return f;
    }

    return NULL;
}

struct uc_hook_func*    ur_hook_func_find_by_addr(uc_runtime_t *r, uint64_t addr)
{
    int i;
    struct uc_hook_func *f;

    for (i = 0, f= r->hooktab.list; i < r->hooktab.counts; i++, f = f->node.next) {
        if (f->address == addr)
            return f;
    }

    return NULL;
}

struct uc_hook_func*    ur_regist_func(uc_runtime_t *t, const char *name, uint32_t offset)
{
    struct uc_hook_func *f;
    if (f = ur_hook_func_find(t, name))
        return f;

    uint32_t v = (uint32_t)ur__malloc(t, &t->rel, 4);
    if (!v) {
        assert(0);
    }

    f = calloc(1, sizeof (f[0]));
    f->name = name;
    f->address = v;
    mlist_add(t->hooktab, f, node);

    uc_mem_write(t->uc, offset, &v, sizeof (v));

    printf("hook func = %s\n", name);
    return f;
}

void            ur_elf_lib_init(uc_runtime_t *t)
{
    Elf32_Shdr *dynsymsh = elf32_shdr_get((Elf32_Ehdr *)t->fdata, SHT_DYNSYM);
    Elf32_Shdr *sh = elf32_shdr_get_by_name((Elf32_Ehdr *)t->fdata, ".rel.dyn");
    Elf32_Rel *rel;
    int i, count, type, symind;
    Elf32_Sym *sym;
    const char *name;

    if (sh->sh_type != SHT_REL) {
        assert(0);
    }

    count = sh->sh_size / sh->sh_entsize;
    for (i = 0; i < count; i++) {
        rel = ((Elf32_Rel *)(t->fdata + sh->sh_offset)) + i;
        type = ELF32_R_TYPE(rel->r_info);
        symind = ELF32_R_SYM(rel->r_info);
        sym = ((Elf32_Sym *)(t->fdata + dynsymsh->sh_offset)) + symind;

        if (sym->st_value) continue;
        name = elf32_sym_name((Elf32_Ehdr *)t->fdata, sym);

        if (name && name[0])
            ur_regist_func(t, name, (uint32_t)(rel->r_offset + t->text.data));
    }
}

