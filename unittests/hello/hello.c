
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char * a(char* Text)
{
    static char secretText[128]={'\0'};
    int count=0;
    int i;
    count = strlen(Text);
    for(i=0;i<count;i++){
        secretText[i]=Text[i]-i-1;
    }
    secretText[i] = '\0';
    return secretText;
}