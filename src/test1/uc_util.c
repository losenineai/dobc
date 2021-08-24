

#include "uc_util.h"
#include "mcore/mcore.h"
#include <assert.h>

#define align(x,n)              ((x + n - 1) & ~(n - 1))

static void             ur_elf_addr_fix(uc_runtime_t *t);
static uc_pos_t         ur__alloc(uc_runtime_t *r, struct uc_area *area, int size, int align_size);

#define ALIGN_FILL(shift)               shift,  1 << (shift), (~((1 << SHIFT) -1))

struct uc_area  default_areas[] = {
    { UC_AREA_TEXT, 0,  0x10000,    0, 1   * MB, UC_PROT_READ | UC_PROT_EXEC,  12, 1 << 12, 0 },
    { UC_AREA_DATA, 0,  0x200000,   0, 128 * KB, UC_PROT_READ | UC_PROT_WRITE, 12, 1 << 12, 0 },
    { UC_AREA_BSS,  0,  0x300000,   0, 4   * KB, UC_PROT_READ | UC_PROT_WRITE, 12, 1 << 12, 0 },
    { UC_AREA_HEAP, 0,  0x400000,   0, 1   * MB, UC_PROT_READ | UC_PROT_WRITE, 12, 1 << 12, 0 },
    { UC_AREA_STACK,0,  0x500000,   0, 128 * KB, UC_PROT_READ | UC_PROT_WRITE, 12, 1 << 12, 0 },
};

static struct uc_hook_func*    ur_relocate_func(uc_runtime_t *t, const char *name, uint32_t offset);

void malloc_cb(uc_runtime_t *t)
{
    int r0;

    uc_reg_read(t->uc, UC_ARM_REG_R0,  &r0);

    r0 = (int)ur_malloc(t, r0);

    uc_reg_write(t->uc, UC_ARM_REG_R0, &r0);
}

void free_cb(uc_runtime_t *t)
{
    int r0;

    uc_reg_read(t->uc, UC_ARM_REG_R0,  &r0);

    ur_free((void *)r0);
}

void memcpy_cb(uc_runtime_t *t)
{
    int r0, r1, r2;
    char buf[512];

    uc_reg_read(t->uc, UC_ARM_REG_R0, &r0);
    uc_reg_read(t->uc, UC_ARM_REG_R1, &r1);
    uc_reg_read(t->uc, UC_ARM_REG_R2, &r2);

    uc_mem_read(t->uc, r1, buf, r2);
    uc_mem_write(t->uc, r0, buf, r2);
}

void default_cb(uc_runtime_t *t)
{
    struct uc_hook_func *cur = ur_get_cur_func(t);

    printf("function %s not implemtation", cur->name);

    exit(0);
}

int             ur_stdlib_regist(uc_runtime_t *t)
{
    t->hooktab._malloc = ur_alloc_func(t, "malloc",  malloc_cb, t);
    t->hooktab._free = ur_alloc_func(t, "free",  malloc_cb, t);
    t->hooktab._memcpy = ur_alloc_func(t, "memcpy",  malloc_cb, t);
    return 0;
}

uc_runtime_t*   uc_runtime_new(uc_engine *uc, const char *soname, int stack_size, int heap_size)
{
    uc_runtime_t *ur;
    uc_err err;
    struct uc_area *area;
    int i;

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

    for (i = 0; i < count_of_array(default_areas); i++) {
        area = &ur->areas[default_areas[i].type];
        *area = default_areas[i];

        if ((err = uc_mem_map(uc, area->begin, area->size, area->perms))) {
            print_err ("err: %s  failed with (%d:%s)uc_mem_map. %s:%d\r\n",  __func__, err, uc_strerror(err), __FILE__, __LINE__);
            return NULL;
        }

        area->end = area->begin + area->size - 1;
    }

    int memlen;
    uint8_t *memdata = elf_load_binary(soname, &memlen);

    ur->elf_load_addr = ur__alloc(ur, ur_area_text(ur), memlen, ur_area_text(ur)->align_size);

    //uc_mem_write(uc, ur->elf_load_addr, ur->fdata, ur->flen);
    uc_mem_write(uc, ur->elf_load_addr, memdata, memlen);

    free(memdata);

    int stack_guard = rand();
    ur_symbol_add(ur, "__stack_chk_guard", UR_SYMBOL_DATA, &stack_guard, sizeof (stack_guard));
    ur_stdlib_regist(ur);

    ur_elf_addr_fix(ur);

    return ur;
}

static uc_pos_t            ur__alloc(uc_runtime_t *r, struct uc_area *area, int size, int align_size)
{
    int ind = area->index;

    if ((ind + size) > area->size)
        return 0;

    if (!align_size)
        align_size = 1;

    area->index += align(size, align_size);

    return area->begin + ind;
}

uc_pos_t            ur_malloc(uc_runtime_t *r, int size)
{
    return ur__alloc(r, ur_area_heap(r), size, 8);
}

uc_pos_t           ur_calloc(uc_runtime_t *uc, int elmnum, int elmsiz)
{
    return 0;
}

uc_pos_t            ur_text_start(uc_runtime_t *r)
{
    return ur_area_text(r)->begin;
}

uc_pos_t        ur_text_end(uc_runtime_t *r)
{
    return ur_area_text(r)->end;
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
    return ur_area_stack(r)->begin;
}

uc_pos_t        ur_stack_end(uc_runtime_t *r)
{
    return ur_area_stack(r)->end;
}

uc_pos_t            ur_symbol_addr(uc_runtime_t *r, const char *symname)
{
    Elf32_Sym *sym = elf32_sym_get_by_name((Elf32_Ehdr *)r->fdata, symname);

    return r->elf_load_addr + sym->st_value;
}

struct ur_symbol*   ur_symbol_find(uc_runtime_t *r, const char *symname, enum ur_symbol_type type)
{
    int i;
    struct ur_symbol *sym;

    for (i = 0, sym = r->symtab.list; i < r->symtab.counts; i++, sym = sym->node.next) {
        if (!strcmp(sym->name, symname) && (sym->type == type))
            return sym;
    }

    return NULL;
}

struct ur_symbol*   ur_symbol_add(uc_runtime_t *r, const char *symname, int type, void *data, int size)
{
    struct ur_symbol *sym = ur_symbol_find(r, symname, type);
    if (sym)
        return sym;

    sym = calloc(1, sizeof (sym[0]) + strlen(symname));
    if (!sym)
        return NULL;

    strcpy(sym->name, symname);
    sym->type = type;

    if (type == UR_SYMBOL_DATA) {
        sym->addr = ur__alloc(r, ur_area_data(r), size, 4);
        uc_mem_write(r->uc, sym->addr, data, size);
    }

    mlist_add(r->symtab, sym, node);

    return sym;
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

void            ur_elf_do_rel_sym_fix(uc_runtime_t *t, Elf32_Rel *rel)
{
    Elf32_Shdr *dynsymsh = elf32_shdr_get((Elf32_Ehdr *)t->fdata, SHT_DYNSYM);

    uint32_t global_offset;
    int type, symind;
    Elf32_Sym *sym;
    struct ur_symbol *ur_sym;
    const char *name;

    type = ELF32_R_TYPE(rel->r_info);
    symind = ELF32_R_SYM(rel->r_info);
    sym = ((Elf32_Sym *)(t->fdata + dynsymsh->sh_offset)) + symind;
    name = elf32_sym_name((Elf32_Ehdr *)t->fdata, sym);

    if (ELF32_ST_TYPE(sym->st_info) == STT_OBJECT) {
        if (sym->st_value) {
            global_offset = sym->st_value + (uint32_t)t->elf_load_addr;
            uc_mem_write(t->uc, t->elf_load_addr + rel->r_offset, (void *)&global_offset, sizeof (global_offset));
        }
        else if (ur_sym = ur_symbol_find(t, name, UR_SYMBOL_DATA)) {
            global_offset = (uint32_t)ur_sym->addr;
            uc_mem_write(t->uc, t->elf_load_addr + rel->r_offset, (void *)&global_offset, sizeof (global_offset));
        }
    }
    else if (ELF32_ST_TYPE(sym->st_info) == STT_FUNC) {
        if (sym->st_value) return;

        if (name && name[0])
            ur_relocate_func(t, name, (uint32_t)(rel->r_offset + t->elf_load_addr));
    }
}

static void            ur_elf_addr_fix(uc_runtime_t *t)
{
    Elf32_Shdr *dynsymsh = elf32_shdr_get((Elf32_Ehdr *)t->fdata, SHT_DYNSYM);
    Elf32_Shdr *sh = elf32_shdr_get_by_name((Elf32_Ehdr *)t->fdata, ".rel.dyn");
    Elf32_Rel *rel;
    int i, count;

    if (sh->sh_type != SHT_REL) {
        assert(0);
    }

    count = sh->sh_size / sh->sh_entsize;
    for (i = 0; i < count; i++) {
        rel = ((Elf32_Rel *)(t->fdata + sh->sh_offset)) + i;
        ur_elf_do_rel_sym_fix(t, rel);
    }

    sh = elf32_shdr_get_by_name((Elf32_Ehdr *)t->fdata, ".rel.plt");
    if (sh->sh_type != SHT_REL) {
        assert(0);
    }

    count = sh->sh_size / sh->sh_entsize;
    for (i = 0; i < count; i++) {
        rel = ((Elf32_Rel *)(t->fdata + sh->sh_offset)) + i;
        ur_elf_do_rel_sym_fix(t, rel);
    }
}

struct uc_hook_func*    ur_relocate_func(uc_runtime_t *t, const char *name, uint32_t offset)
{
    struct uc_hook_func *f = ur_alloc_func(t, name, default_cb, NULL);

    int v = (int)f->address;

    uc_mem_write(t->uc, offset, &v, sizeof (v));

    printf("hook func = %s\n", name);
    return f;
}


struct uc_hook_func*    ur_alloc_func(uc_runtime_t *t, const char *name, void (* cb)(void *user_data), void *user_data)
{
    struct uc_hook_func *f;
    if ((f = ur_hook_func_find(t, name))) {
        if (cb && (f->cb == default_cb)) {
            f->cb = cb;
            f->user_data = user_data;
        }
        return f;
    }

    f = calloc(1, sizeof (f[0]) + strlen(name));
    if (!f)
        return NULL;

    strcpy(f->name, name);
    f->address = ur__alloc(t, ur_area_text(t), 4, 4);
    if (!f->address) {
        free(f);
        return NULL;
    }
    f->cb = cb;
    f->user_data = user_data;

    mlist_add(t->hooktab, f, node);

    return f;
}

void            ur_set_priv_data(uc_runtime_t *r, void *priv)
{
    r->priv_data = priv;
}

void*           ur_get_priv_data(uc_runtime_t *r)
{
    return r->priv_data;
}

int ur_reg_read_batch(uc_runtime_t *r, int *ids, int *vals, int count)
{
    int i;

    for (i = 0; i < count; i++) {
        uc_reg_read(r->uc, ids[i], vals + i);
    }

    return 0;
}

void                    ur_set_cur_func(uc_runtime_t *r, struct uc_hook_func *f)
{
    r->hooktab.cur = f;
}

struct uc_hook_func*    ur_get_cur_func(uc_runtime_t *r)
{
    return r->hooktab.cur;
}
