
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>


int test_hello(const char **so)
{
    void *handle[2];
    char *(*hello[2])(char *, char *);

    handle[0] = dlopen(so[0], RTLD_LAZY);
    handle[1] = dlopen(so[1], RTLD_LAZY);
    if (!handle[0] || !handle[1])
        return -1;

    hello[0] = dlsym(handle[0], "hello");
    hello[1] = dlsym(handle[1], "hello");

    char buf[2][128];
    hello[0]("test", buf[0]);
    hello[1]("test", buf[1]);

    return strcmp(buf[0], buf[1]);
}

int main(int argc, char **argv)
{
    const char *so[2] = {"hello.so", "hello.so.decode"};

    return test_hello(so);
}