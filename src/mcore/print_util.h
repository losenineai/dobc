#ifndef __print_util_h__
#define __print_util_h__

#ifdef __cplusplus
extern "C" {
#endif


#include <stdio.h>

#define PRINT_LEVEL_ERR         1
#define PRINT_LEVEL_WARN        2
#define PRINT_LEVEL_TAG         3
#define PRINT_LEVEL_INFO        4
#define PRINT_LEVEL_DEBUG       5
#define PRINT_LEVEL_DETAIL      6

    extern int print_level;

#if defined(__DEBUG)
#define print_err(fmt,...)      if (print_level >= print_level_err)     printf("[%s] err: "    func_format_s fmt " %s:%d\n", mtime2s(0), func_format(), ##__VA_ARGS__, __FILE__, __LINE__)
#define print_warn(fmt,...)     if (print_level >= print_level_warn)    printf("[%s] warn: "   func_format_s fmt " %s:%d\n", mtime2s(0), func_format(), ##__VA_ARGS__, __FILE__, __LINE__)
#define print_tag(fmt,...)      if (print_level >= print_level_tag)     printf("[%s] tag: "    func_format_s fmt " %s:%d\n", mtime2s(0), func_format(), ##__VA_ARGS__, __FILE__, __LINE__)
#define print_info(fmt,...)     if (print_level >= print_level_info)    printf("[%s] info: "   func_format_s fmt " %s:%d\n", mtime2s(0), func_format(), ##__VA_ARGS__, __FILE__, __LINE__)
#define print_debug(fmt,...)    if (print_level >= print_level_debug)   printf("[%s] debug: "  func_format_s fmt " %s:%d\n", mtime2s(0), func_format(), ##__VA_ARGS__, __FILE__, __LINE__)
#define print_detail(fmt,...)   if (print_level >= print_level_detail)  printf("[%s] detail: " func_format_s fmt " %s:%d\n", mtime2s(0), func_format(), ##__VA_ARGS__, __FILE__, __LINE__)
#else
#define print_err(fmt,...)      if (print_level >= PRINT_LEVEL_ERR)     printf("err: "    fmt " \n", ##__VA_ARGS__)
#define print_warn(fmt,...)     if (print_level >= PRINT_LEVEL_WARN)    printf("warn: "   fmt " \n", ##__VA_ARGS__)
#define print_tag(fmt,...)      if (print_level >= PRINT_LEVEL_TAG)     printf("tag: "    fmt " \n", ##__VA_ARGS__)
#define print_info(fmt,...)     if (print_level >= PRINT_LEVEL_INFO)    printf("info: "   fmt " \n", ##__VA_ARGS__)
#define print_debug(fmt,...)    if (print_level >= PRINT_LEVEL_DEBUG)   printf("debug: "  fmt " \n", ##__VA_ARGS__)
#define print_detail(fmt,...)   if (print_level >= PRINT_LEVEL_DETAIL)  printf("detail: " fmt " \n", ##__VA_ARGS__)
#endif

    void print_level_set(int level);

#ifdef __cplusplus
}
#endif

#endif
