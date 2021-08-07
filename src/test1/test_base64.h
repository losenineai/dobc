
#ifndef __test_base64_h__
#define __test_base64_h__

#include "uc_util.h"

int test_base64_encode_init(struct uc_runtime *ur);
int test_base64_encode_on_exit(struct uc_runtime *test);

int test_base64_encode_init1(struct uc_runtime *ur);
int test_base64_encode_on_exit1(struct uc_runtime *ur);

int test_base64_decode_init(struct test_base64 *test, struct uc_runtime *ur);
int test_base64_decode_(struct test_base64 *test);


#endif