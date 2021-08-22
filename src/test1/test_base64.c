
#include "test.h"
#include "test_base64.h"

static int arm_general_regs[16] = {
    UC_ARM_REG_R0,
    UC_ARM_REG_R1,
    UC_ARM_REG_R2,
    UC_ARM_REG_R3,
    UC_ARM_REG_R4,
    UC_ARM_REG_R5,
    UC_ARM_REG_R6,
    UC_ARM_REG_R7,
    UC_ARM_REG_R8,
    UC_ARM_REG_R9,
    UC_ARM_REG_R10,
    UC_ARM_REG_R11,
    UC_ARM_REG_R12,
    UC_ARM_REG_R13,
    UC_ARM_REG_R14,
    UC_ARM_REG_PC
};

struct test_base64 {
    int regs[16];
};

#define TEST_PLAIN              "hello, world"
#define TEST_CIPHER             "aGVsbG8sIHdvcmxk"

int test_base64_encode_init(struct uc_runtime *ur)
{
    int sp, r0, r1;
    uc_engine *uc = ur->uc;
    struct uc_hook_func *f;

    struct test_base64 *t = calloc(1, sizeof (t[0]));

    if (!t)
        return -ENOMEM;

    ur_set_priv_data(ur, t);

    sp = (int)ur_stack_end(ur) + 1;
    uc_reg_write(uc, UC_ARM_REG_SP, &sp);

    r0 = ur_string32(ur, TEST_PLAIN);
    uc_reg_write(uc, UC_ARM_REG_R0, &r0);

    r1 = (int)ur_malloc(ur, 100);
    uc_reg_write(uc, UC_ARM_REG_R1, &r1);

    f = ur_alloc_func(ur, "main_exit", test_base64_encode_on_exit, ur);
    uc_reg_write(uc, UC_ARM_REG_LR, &f->address);

    ur_reg_read_batch(ur, arm_general_regs, t->regs, count_of_array(t->regs));

    test_dump_regs(ur);

    printf("start run====================================\n");

    return 0;
}

int test_base64_encode_on_exit(struct uc_runtime *r)
{
    struct test_base64 *test = ur_get_priv_data(r);
    int r0;
    char buf[128];

    uc_emu_stop(r->uc);

    uc_reg_read(r->uc, UC_ARM_REG_R0,  &r0);

    uc_mem_read(r->uc, test->regs[1], buf, sizeof (buf));

    if (strcmp(buf, TEST_CIPHER)) {
        printf("base64_encode test failure\n");
    }
    else {
        printf("base64_encode test success, out[%s]\n", buf);
    }

    return 0;
}

int test_base64_encode_init1(struct uc_runtime *ur)
{
    int sp, r0, r1;
    uc_engine *uc = ur->uc;
    struct uc_hook_func *f;

    struct test_base64 *t = calloc(1, sizeof (t[0]));

    if (!t)
        return -ENOMEM;

    ur_set_priv_data(ur, t);

    sp = (int)ur_stack_end(ur) + 1;
    uc_reg_write(uc, UC_ARM_REG_SP, &sp);

    r0 = ur_string32(ur, "h");
    uc_reg_write(uc, UC_ARM_REG_R0, &r0);

    r1 = (int)ur_malloc(ur, 100);
    uc_reg_write(uc, UC_ARM_REG_R1, &r1);

    f = ur_alloc_func(ur, "main_exit", test_base64_encode_on_exit1, ur);
    uc_reg_write(uc, UC_ARM_REG_LR, &f->address);

    ur_reg_read_batch(ur, arm_general_regs, t->regs, count_of_array(t->regs));

    test_dump_regs(ur);

    printf("start run====================================\n");

    return 0;
}

int test_base64_encode_on_exit1(struct uc_runtime *r)
{
    struct test_base64 *test = ur_get_priv_data(r);
    int r0;
    char buf[128];

    uc_emu_stop(r->uc);

    uc_reg_read(r->uc, UC_ARM_REG_R0,  &r0);

    uc_mem_read(r->uc, test->regs[1], buf, sizeof (buf));

    if (strcmp(buf, "aA==")) {
        printf("base64_encode test failure[%d:%s]\n", r0, buf);
    }
    else {
        printf("base64_encode test success, out[%s]\n", buf);
    }

    return 0;
}

int test_base64_decode_init(struct uc_runtime *ur)
{
    int sp, r0, r1;
    uc_engine *uc = ur->uc;
    struct uc_hook_func *f;

    struct test_base64 *t = calloc(1, sizeof (t[0]));

    if (!t)
        return -ENOMEM;

    ur_set_priv_data(ur, t);

    sp = (int)ur_stack_end(ur) + 1;
    uc_reg_write(uc, UC_ARM_REG_SP, &sp);

    r0 = ur_string32(ur, TEST_CIPHER);
    uc_reg_write(uc, UC_ARM_REG_R0, &r0);

    r1 = (int)ur_malloc(ur, 100);
    uc_reg_write(uc, UC_ARM_REG_R1, &r1);

    f = ur_alloc_func(ur, "main_exit", test_base64_encode_on_exit, ur);
    uc_reg_write(uc, UC_ARM_REG_LR, &f->address);

    ur_reg_read_batch(ur, arm_general_regs, t->regs, count_of_array(t->regs));

    test_dump_regs(ur);

    printf("start run====================================\n");

    return 0;
}

int test_base64_decode_on_exit(struct uc_runtime *r)
{
    struct test_base64 *test = ur_get_priv_data(r);
    int r0;
    char buf[128];

    uc_emu_stop(r->uc);

    uc_reg_read(r->uc, UC_ARM_REG_R0,  &r0);

    uc_mem_read(r->uc, test->regs[1], buf, sizeof (buf));

    if (strcmp(buf, TEST_PLAIN)) {
        printf("base64_decode test failure, out[%s]\n", buf);
    }
    else {
        printf("base64_decode test success, out[%s]\n", buf);
    }

    return 0;
}
