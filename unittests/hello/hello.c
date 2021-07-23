
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char * hello(char *text, char *outbuf) __attribute((__annotate__(("bcf,fla,sub"))))
{
    sprintf(outbuf, "hello, %s", text);

    return outbuf;
}
