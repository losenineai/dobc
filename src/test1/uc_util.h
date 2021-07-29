
#ifndef __uc_util_h__
#define __uc_util_h__

#include "unicorn/unicorn.h"

#define KB      1024
#define MB      (1024 * KB)

#define NONE "\033[m"
#define RED "\033[0;32;31m"
#define LIGHT_RED "\033[1;31m"
#define GREEN "\033[0;32;32m"
#define LIGHT_GREEN "\033[1;32m"
#define BLUE "\033[0;32;34m"
#define LIGHT_BLUE "\033[1;34m"
#define DARY_GRAY "\033[1;30m"
#define CYAN "\033[0;36m"
#define LIGHT_CYAN "\033[1;36m"
#define PURPLE "\033[0;35m"
#define LIGHT_PURPLE "\033[1;35m"
#define BROWN "\033[0;33m"
#define YELLOW "\033[1;33m"
#define LIGHT_GRAY "\033[0;37m"
#define WHITE "\033[1;37m"

#define print_red()             printf(RED)
#define print_light_red()       printf(LIGHT_RED)
#define print_green()           printf(GREEN)
#define print_light_green()     printf(LIGHT_GREEN)
#define print_blue()            printf(BLUE)
#define print_light_blue()      printf(LIGHT_BLUE)
#define print_dark_gray()       printf(DARK_GRAY)
#define print_cyan()            printf(CYAN)
#define print_light_cyan()      printf(LIGHT_CYAN)
#define print_purple()          printf(PURPLE)
#define print_light_purple()    printf(LIGHT_PURPLE)
#define print_brown()           printf(BROWN)
#define print_yellow()          printf(YELLOW)
#define print_light_gray()      printf(LIGHT_GRAY)
#define print_white()           printf(WHITE)

typedef uint64_t            uc_pos_t;
typedef uint64_t*           uc_ptr;
typedef long                pos_t;

typedef struct uc_runtime   uc_task_struct;

typedef enum ur_error {
    UR_SUCCESS  = 0,
    UR_EINVAL   = 1,
    UR_ENOMEM   = 2,
} ur_error;

struct uc_area {
    int         index;
    uc_pos_t    data;
    uc_pos_t    end;
    int         size;
};

struct uc_hook_func;

struct uc_hook_func {
    struct {
        struct uc_hook_func     *next;
        struct uc_hook_func     *prev;
    } node;
    const char *name;
    uint64_t    address;
};

/*
FIXME:
1. uc_runtime 改为 uc_task_struct
2. 我们没有区分线程和进程


*/
typedef struct uc_runtime {
    uc_engine*  uc;
    int         err;

    struct uc_area  heap;
    struct uc_area  stack;
    struct uc_area  text;
    struct uc_area  rel;

    uint8_t*    fdata;
    int         flen;

    /* 线程集 */
    struct {
        int counts;
    } task_heads;

    struct {
        int     counts;
        struct uc_hook_func *list;
    } hooktab;

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

uc_pos_t        ur_symbol_addr(uc_runtime_t *r, const char *sym);

void*           uc_vir2phy(uc_pos_t t);

struct uc_hook_func*    ur_hook_func_find(uc_runtime_t *r, const char *name);
struct uc_hook_func*    ur_hook_func_find_by_addr(uc_runtime_t *r, uint64_t addr);


#endif