#include <stdio.h>
#include "mmpool.h"
#include <vector>
using std::vector;
int main()
{
    size_t sz = mmpool_calc_size(1000000 , 32, E_MMPOOL_TYPE_FIXED_BLOCK_LIST );// E_MMPOOL_TYPE_FIXED_BMP);
    printf("size:%zu \n",sz);
    char * szBuffer = (char*)malloc(sz);
    mmpool_t * mp = mmpool_create(szBuffer,
        sz,
        E_MMPOOL_TYPE_FIXED_BMP,
        32,1000000);
    if(!mp)
    {
        puts("err");
        return 1;    
    }
    std::vector<int> vec;

    printf("init free:%zd\n",mmpool_stat_free(mp));
    printf("init used:%zd\n",mmpool_stat_used(mp));
    for(int i = 0 ;i < 1000000; ++i)
    {
        int id =  mmpool_alloc(mp);
        if(rand()%100 > 50)
        {
           mmpool_free(mp, id); 
        }
        else
        {
            if(id > 0)
            {
                vec.push_back(id);
            }
        }
    }

    printf("dirty free:%zd\n",mmpool_stat_free(mp));
    printf("dirty used:%zd\n",mmpool_stat_used(mp));
 
   for(int i = 0;i < 10000; ++i)
   {
        int id = mmpool_alloc(mp);
        if(id > 0)
        {
            vec.push_back(id);
        }
   }
   for(int i = 0;i < vec.size(); ++i)
   {
       mmpool_free(mp, vec[i]);
   }

    printf("destroy free:%zd\n",mmpool_stat_free(mp));
    printf("destroy used:%zd\n",mmpool_stat_used(mp));
    return 0;
}





