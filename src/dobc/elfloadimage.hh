
#include "loadimage.hh"

typedef map<Address, LoadImageFunc *>   addrtab;
typedef map<string, LoadImageFunc *>   nametab;

class ElfLoadImage : public LoadImageB {
    long long baseaddr;

    addrtab     addrtab;
    nametab     nametab;
    addrtab::iterator curit;

    int is64bit;
    FILE *fp;
    int cur_sym;
    AddrSpace *codespace;

public:
    unsigned char*  filedata;
    int				filelen;

    ElfLoadImage(const char *filename);
    virtual ~ElfLoadImage();
    void        init();

    void setCodeSpace(AddrSpace *a) { codespace = a; }
    /* 再扫描elf的符号表时，会出现数据紧跟在代码区域后面，这部分的数据不应该在解析了，
    我们这里假设这个跟随的数据区是4字节为一个单位的，所以我们用一个 filelen/32 大小的bit数组来
    标识是否是数据还是代码 */
#define DATA_TOP                -1
#define DATA_IN_CODE            0
#define DATA_IN_BSS             1
    virtual int loadFill(uint1 *ptr,int4 size,const Address &addr);
    virtual string getArchType(void) const { return is64bit?"Elf64":"Elf32"; }
    virtual bool getNextSymbol(LoadImageFunc &record); 
    virtual void adjustVma(long adjust) { }
    int getSymbol(const string &name, LoadImageFunc &record);
    int saveSymbol(const char *symname, int size);
    int addSymbol(const Address &addr, int size);
    void saveFile(const char *filename);
    addrtab::iterator   beginSymbol() { return addrtab.begin();  }
    addrtab::iterator   endSymbol() { return addrtab.end();  }

    intb    read_val(intb addr, int siz);
};
