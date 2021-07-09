#include "vm.h"
#include "elfloadimage.hh"
#include <assert.h>

void ElfLoadImage::init()
{
    assert(codespace);

    int count = elf32_sym_count((Elf32_Ehdr *)filedata), i;
    LoadImageFunc *func;
    Elf32_Shdr *dynsymsh, *link_sh;
    Elf32_Ehdr *hdr = (Elf32_Ehdr *)filedata;
    char *name;

    dynsymsh = elf32_shdr_get(hdr, SHT_DYNSYM);

    link_sh = (Elf32_Shdr *)(filedata + hdr->e_shoff) + dynsymsh->sh_link;

    for (i = 0; i < count; i++) {
        Elf32_Sym  *sym = elf32_sym_geti((Elf32_Ehdr *)filedata, i);
        func = new LoadImageFunc();
        name = (char *)filedata + (link_sh->sh_offset + sym->st_name);

        func->address = Address(codespace, sym->st_value);
        func->name = string(name);
        func->size = sym->st_size;

        addrtab[func->address] = func;
        nametab[func->name] = func;
    }
}

typedef struct plt_entry {
    uint32_t    adr;
    uint32_t    add;
    uint32_t    ldr;
} plt_entry_t;

void ElfLoadImage::init_plt()
{
    Elf32_Shdr *plt = elf32_shdr_get_by_name(hdr, ".plt");
    Elf32_Shdr *relplt = elf32_shdr_get_by_name(hdr, ".rel.plt");
    Elf32_Shdr *dynsymsh = elf32_shdr_get_by_name(hdr, ".dynsym");
    Elf32_Rel *rel;
    Elf32_Sym *sym;
    int type, symind, offset;
    const char *name;
    struct plt_entry *ent;

    LoadImageSymbol *imgsym;

    int i, imm;
    int count = relplt->sh_size / relplt->sh_entsize;

    for (i = 0; i < count; i++) {
        rel = ((ElfW(Rel) *)(filedata + relplt->sh_offset)) + i;
        type = ELFW(R_TYPE)(rel->r_info);
        symind = ELFW(R_SYM)(rel->r_info);
        sym = ((ElfW(Sym *))(filedata + dynsymsh->sh_offset)) + symind;
        name = elf32_sym_name(hdr, sym);

        imgsym = new LoadImageSymbol();

        imgsym->address = Address(codespace, rel->r_offset);
        imgsym->type = SYM_IMPORT_PTR;
        imgsym->name = string(name) + "_ptr";

        addrtab[imgsym->address] = imgsym;
        nametab[imgsym->name] = imgsym;
    }


    /*
plt:0000D590                                             ; ORG 0xD590
.plt:0000D590                                             CODE32
.plt:0000D590 04 E0 2D E5                                 PUSH            {LR}
.plt:0000D594 04 E0 9F E5                                 LDR             LR, =(_GLOBAL_OFFSET_TABLE_ - 0xD5A0)
.plt:0000D598 0E E0 8F E0                                 ADD             LR, PC, LR ; _GLOBAL_OFFSET_TABLE_
.plt:0000D59C 08 F0 BE E5                                 LDR             PC, [LR,#8]! ; dword_0
.plt:0000D59C                             ; ---------------------------------------------------------------------------
.plt:0000D5A0 50 98 03 00                 off_D5A0        DCD _GLOBAL_OFFSET_TABLE_ - 0xD5A0

plt section的头，都有20个字节的头，这20个字节的头不知道是干什么的
*/

#define MATCH_INST(a,pat)           ((a & (pat)) == pat)
    uint8_t *plt_data = filedata + plt->sh_offset;
    plt_data += 20;

    count = (plt->sh_size - 20) / sizeof(plt_entry);

    for (i = 0; i < count; i++) {
        ent = ((plt_entry_t *)plt_data) + i;

        if (!MATCH_INST(ent->adr, 0xe28f0000)   // A8.8.12 A1
            || !MATCH_INST(ent->add, 0xe28c0000) // A8.8.5 A1
            || !MATCH_INST(ent->ldr, 0xe4100000))  { // A8.8.63 A1 
            vm_error("un-expect plt entry pattern");
        }

        offset = ((unsigned char *)ent - filedata) + 8;
        /* A5.2.4 ARMExpandIMM，
        
        因为我没有去完整实现这个函数，所以这里直接简单处理了下
        */
        imm = ent->add & 0xfff;
        uint32_t unrotate_value = imm & 0xff;
        uint32_t r = imm >> 8;
        uint32_t imm32 = (unrotate_value >> r) | (unrotate_value << (32 - r));

        imm = ent->ldr & 0xfff;
        imm32 = imm32 + (ent->ldr & (1 << 24)) ? imm:-imm;
    }
}
 
ElfLoadImage::ElfLoadImage(const string &filename):LoadImageB(filename)
{
    filedata = (unsigned char *)file_load(filename.c_str(), (int *)&filelen);
    hdr = (Elf32_Ehdr *)filedata;

    if (!filedata) 
        vm_error("ElfLoadImage() failed open [%s]", filename);

    cur_sym = -1;

    //init_plt();
}

ElfLoadImage::~ElfLoadImage()
{
    file_unload((char *)filedata);
}

int ElfLoadImage::loadFill(uint1 *ptr, int size, const Address &addr) 
{
    unsigned start = (unsigned)addr.getOffset();
    Elf32_Shdr *sh;

    sh = elf32_shdr_get_by_addr(hdr, start);

    if (sh && (sh->sh_flags & SHF_WRITE))
        return -1;

    if ((start + size) > filelen) {
        /* FIXME: 我们对所有访问的超过空间的地址都返回 0xaabbccdd，这里不是BUG，是因为我们载入so的时候，是直接平铺着载入的
        但是实际在程序加载so的时候，会填充很多结构，并做一些扩展 */
        return -1;
    }

    memcpy(ptr, filedata + start, size);
    return 0;
}

bool ElfLoadImage::getNextSymbol(LoadImageFunc &record) 
{
    Elf32_Shdr *dynsymsh, *link_sh;
    Elf32_Sym *sym;
    Elf32_Ehdr *hdr = (Elf32_Ehdr *)filedata;
    int num;
    const char *name;

    cur_sym++;

    dynsymsh = elf32_shdr_get((Elf32_Ehdr *)filedata, SHT_DYNSYM);
    if (!dynsymsh) 
        vm_error("file[%s] have not .dymsym section", filename.c_str());

    link_sh = (Elf32_Shdr *)(filedata + hdr->e_shoff) + dynsymsh->sh_link;

    num = dynsymsh->sh_size / dynsymsh->sh_entsize;
    if (cur_sym >= num) {
        cur_sym = -1;
        return false;
    }

    sym = (Elf32_Sym *)(filedata + dynsymsh->sh_offset) + cur_sym;
    name = (char *)filedata + (link_sh->sh_offset + sym->st_name);

    record.address = Address(codespace, sym->st_value);
    record.name = string(name);
    record.size = sym->st_size;

    return true;
}

int ElfLoadImage::getSymbol(const string &name, LoadImageFunc &record)
{
    LoadImageFunc *func = nametab[name];
    if (func)
        record = *func;

    return func ? 0:-1;;
}

int ElfLoadImage::saveSymbol(const char *symname, int size)
{
    Elf32_Sym *sym = elf32_sym_find2((Elf32_Ehdr *)filedata, symname);

    if (!sym)
        return -1;

    sym->st_size = size;

    return 0;
}

int ElfLoadImage::addSymbol(const Address &addr, int size)
{
    LoadImageFunc *func;
    char buf[128];

    sprintf(buf, "sub_%llx", addr.getOffset());

    func = new LoadImageFunc();

    func->address = addr;
    func->name = string(buf);
    func->size = size;

    addrtab[func->address] = func;
    nametab[func->name] = func;

    return 0;
}

void ElfLoadImage::saveFile(const string &filename)
{
    file_save(filename.c_str(), (char *)filedata, filelen);
}

intb    ElfLoadImage::read_val(intb addr, int siz)
{
    if (siz == 1)
        return filedata[addr];
    else if (siz == 2)
        return *(short *)(filedata + addr);
    else if (siz == 4)
        return *(int *)(filedata + addr);
    else
        return *(intb *)(filedata + addr);
}
