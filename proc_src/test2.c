#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void)
{
    int    local_nTemp   = 0;
    char** local_ptrTemp = NULL;

    local_ptrTemp = (char**)malloc(sizeof(char*) * 200 );

    for ( local_nTemp = 0; local_nTemp < 200; local_nTemp++ )
    {
        local_ptrTemp[local_nTemp] = (char*)malloc(sizeof(char) * 32);
        memset( local_ptrTemp[local_nTemp], 0x00, sizeof(char) * 32 );

        snprintf(local_ptrTemp[local_nTemp], sizeof(char) * 32, "%d TESTDDWDWDWDWDWDWDWDWDDWDFFGG", local_nTemp);
    }


    for ( local_nTemp = 0; local_nTemp < 200; local_nTemp++ )
    {
        printf("[%s] \n", local_ptrTemp[local_nTemp] );
    }

    return 0;

}


