#pragma once
#include <stdlib.h>

enum    E_MMPOOL_TYPE
{
    E_MMPOOL_TYPE_FIXED_BMP = 1, //fixed size with bitmap
    E_MMPOOL_TYPE_FIXED_BLOCK_LIST = 2, //fixed size with block list
    E_MMPOOL_TYPE_FLEX_BLOCK_LIST = 3,  //
    E_MMPOOL_TYPE_FLEX_BUDDY_SYSTEM = 4,  //      
};

struct mmpool_t ;
mmpool_t *   mmpool_create(void * buffer, int sz, int strategy_type,
                int block_size, int block_num);
mmpool_t *   mmpool_attach(void * buffer, int sz);
size_t       mmpool_calc_size(int block_num, int block_size, int strategy_type );
int          mmpool_destroy(mmpool_t * pool);


int          mmpool_alloc(mmpool_t * pool ,int block_num = 1);//>0
int          mmpool_free(mmpool_t * pool ,int id);
void *       mmpool_addr(mmpool_t * pool ,int id);
int          mmpool_id(mmpool_t * pool ,void *  p);

//traverse
int          mmpool_stat_free(mmpool_t * pool);
int          mmpool_stat_used(mmpool_t * pool);
int          mmpool_begin(mmpool_t * pool);
int          mmpool_next(mmpool_t * pool, int id);




