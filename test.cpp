#include <stdio.h>
#include "mmpool.h"

int main()
{
    static char szBuffer[1024*1024*4];


    
    mmpool_t * mp = mmpool_create(szBuffer, 
        sizeof(szBuffer),
        E_MMPOOL_TYPE_FIXED_BMP,
        16,50);
    if(!mp)
    {
        puts("err");
        return 1;    
    }

    int id =  mmpool_alloc(mp);
    void *p  =  mmpool_addr(mp,id);
    printf("alloc:%p\n", p);
    return 0;
}





