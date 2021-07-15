
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char * hello(char *text, char *outbuf)
{
    sprintf(outbuf, "hello, %s", text);

    return outbuf;
}