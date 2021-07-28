
#ifndef __uc_util_h__
#define __uc_util_h__

#include "unicorn/unicorn.h"

#define KB      1024
#define MB      (1024 * KB)

typedef uint64_t            uc_pos_t;
typedef uint64_t*           uc_ptr;
typedef long                pos_t;

typedef struct uc_runtime   uc_task_struct;

typedef enum ur_error {
    UR_SUCCESS  = 0,
    UR_EINVAL   = 1,
    UR_ENOMEM   = 2,
} ur_error;

/*
FIXME:
1. uc_runtime 改为 uc_task_struct
2. 我们没有区分线程和进程


*/
typedef struct uc_runtime {
    uc_engine*  uc;
    int         err;

    struct {
        int         index;
        uc_pos_t    data;
        uc_pos_t    end;
        int         size;
    } heap;

    /* 
    FIXME: text这个段的概念是来自于elf，而不属于进程管理
    */
    struct {
        uc_pos_t    data;
        uc_pos_t    end;
        int         size;
    } stack;

    struct {
        uc_pos_t    data;
        uc_pos_t    end;
        int         size;
    } text;

    uint8_t*    fdata;
    int         flen;


    /* 线程集 */
    struct {
        int counts;
    } task_heads;

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


#endif