
#ifndef __uc_util_h__
#define __uc_util_h__

#include "unicorn/unicorn.h"

#define KB      1024
#define MB      (1024 * KB)

typedef uint64_t            uc_pos_t;
typedef uint64_t*           uc_ptr;
typedef long                pos_t;

#define UE_NOMEM            1
#define UE_UC_NOMEM         2
#define UE_INVAL            3

typedef struct uc_runtime   uc_task_struct;

typedef enum ur_error {
    UR_SUCCESS  = 0,
    UR_EINVAL   = 1,
    UR_ENOMEM   = 2,
} ur_error;

enum uc_area_type {
    UC_AREA_TEXT,
    UC_AREA_DATA,
    UC_AREA_BSS,
    UC_AREA_HEAP,
    UC_AREA_STACK,
    UC_AREA_MAX
};

#define count_of_array(a)       (sizeof (a) / sizeof (a[0]))


struct uc_area {
    enum uc_area_type   type;
    int                 index;
    uc_pos_t            begin;
    uc_pos_t            end;
    int                 size;
    uint32_t            perms;

    int                 align_shift;
    int                 align_size;
    int                 align_mask;
};

enum ur_symbol_type {
    UR_SYMBOL_DATA,
    UR_SYMBOL_FUNC
};

struct ur_symbol;
struct ur_symbol {
    struct {
        struct ur_symbol *next;
        struct ur_symbol *prev;
    } node;

    enum ur_symbol_type type;
    uc_pos_t addr;

    char name[1];
};

struct uc_hook_func;

struct uc_hook_func {
    struct {
        struct uc_hook_func     *next;
        struct uc_hook_func     *prev;
    } node;
    uc_pos_t    address;
    char        name[1];
    void*       user_data;
    void(*cb)(void *user_data);
};

/*
High Addresses ---> .----------------------.
                    |      Environment     |
                    |----------------------|
                    |                      |   Functions and variable are declared
                    |         STACK        |   on the stack.
base pointer ->     | - - - - - - - - - - -|
                    |           |          |
                    |           v          |
                    :                      :
                    .                      .   The stack grows down into unused space
                    .         Empty        .   while the heap grows up. 
                    .                      .
                    .                      .   (other memory maps do occur here, such 
                    .                      .    as dynamic libraries, and different memory
                    :                      :    allocate)
                    |           ^          |
                    |           |          |
 brk point ->       | - - - - - - - - - - -|   Dynamic memory is declared on the heap
                    |          HEAP        |
                    |                      |
                    |----------------------|
                    |          BSS         |   Uninitialized data (BSS)
                    |----------------------|   
                    |          Data        |   Initialized data (DS)
                    |----------------------|
                    |          Text        |   Binary code
Low Addresses ----> '----------------------'
*/



/*
FIXME:
1. uc_runtime 改为 uc_task_struct
2. 我们没有区分线程和进程

*/

#define ur_area_text(u)             (&u->areas[UC_AREA_TEXT])
#define ur_area_data(u)             (&u->areas[UC_AREA_DATA])
#define ur_area_bss(u)              (&u->areas[UC_AREA_BSS])
#define ur_area_heap(u)             (&u->areas[UC_AREA_HEAP])
#define ur_area_stack(u)            (&u->areas[UC_AREA_STACK])

typedef struct uc_runtime {
    uc_engine*  uc;
    int         err;

    struct uc_area  areas[UC_AREA_MAX];

    uint8_t*    fdata;
    int         flen;
    uc_pos_t    elf_load_addr;

    /* 线程集 */
    struct {
        int counts;
    } task_heads;

    struct {
        int     counts;
        struct uc_hook_func *list;
    } hooktab;

    struct {
        int     counts;
        struct ur_symbol *list;
    } symtab;

    struct {
        unsigned int trace : 1;
        unsigned int trace_blk : 1;
    } debug;

    char soname[1];
} uc_runtime_t;

uc_runtime_t*   uc_runtime_new(uc_engine *uc, const char *soname, int stack_size, int heap_size);

/*
普通的linux在运行时，实在 虚拟内存(virtual memory or flat memory)上，在经过TLB和MMU以后，转换成phy address

我们在uc_runtime上，运行的函数，最后得到的结果都是 unicorn 上的virtual address
*/
uc_pos_t        ur_malloc(uc_runtime_t *r, int size);
uc_pos_t        ur_calloc(uc_runtime_t *r, int elmnum, int elmsiz);

void            ur_free(void* v);
uc_pos_t        ur_strcpy(uc_runtime_t *r, uc_pos_t dst, uc_pos_t src);

uc_pos_t        ur_text_start(uc_runtime_t *r);
uc_pos_t        ur_text_end(uc_runtime_t *r);

uc_pos_t        ur_stack_start(uc_runtime_t *r);
uc_pos_t        ur_stack_end(uc_runtime_t *r);

/*

我们来看一段代码

const char *str = "hello, world"
int len = strlen(str);
r0 = ur_malloc(len + 1);
ur_strcpy(r0, str);

这个函数有什么问题？

问题在strcpy的，dest是virtual address, str是physical address

我们在转换到target上去运行时，所有内存地址都要变成virtual address

*/
uc_pos_t        ur_string(uc_runtime_t *r, const char *str);
#define ur_string32(r,str)  (int)ur_string(r,str)

struct ur_symbol*   ur_symbol_find(uc_runtime_t *r, const char *symname, enum ur_symbol_type type);
uc_pos_t            ur_symbol_addr(uc_runtime_t *r, const char *sym);
struct ur_symbol*   ur_symbol_add(uc_runtime_t *r, const char *symname, int type, void *data, int size);

void*           uc_vir2phy(uc_pos_t t);

struct uc_hook_func*    ur_hook_func_find(uc_runtime_t *r, const char *name);
struct uc_hook_func*    ur_hook_func_find_by_addr(uc_runtime_t *r, uint64_t addr);

/*
分配一个函数

@return         0       success
                <0      failure, error code
*/
struct uc_hook_func*    ur_alloc_func(uc_runtime_t *r, const char *name, void (* cb)(void *user_data), void *user_data);



#endif