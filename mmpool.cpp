#include "mmpool.h"
#include <strings.h>    


#pragma pack(1)
#define     MMPOOL_MAGIC        (0x1234)
typedef struct mmpool_strategy_fixed_bitmap_t
{
    int         last_alloc_block;
    int         block_num;
    int         free_block_num;
    int         used_block_num;
    int         data_offset_from_this;
    unsigned    char  bmp[1];
}
mmpool_strategy_fixed_bitmap_t;

typedef struct mmpool_t
{
    int    magic;
    int    type;
    int    block_size;
    int    buffer_size;
    int    data_size;
    int    free_size;
    int    used_size;
    union
    {
        mmpool_strategy_fixed_bitmap_t        fixed_bmp;
        //mmpool_strategy_fixed_block_list_t    fixed_block_list;
        //mmpool_strategy_flex_block_list_t     flex_block_list;
        //mmpool_strategy_flex_buddy_system_t   flex_buddy_system;        
    };
} mmpool_t;
#pragma pack()


int mmpool_fixed_bmp_alloc(mmpool_strategy_fixed_bitmap_t * bmp)
{
    if(bmp->used_block_num >= bmp->block_num)
    {        
        return -1;//error full
    }
    //todo optimal with bit
    for(int i = bmp->last_alloc_block + 1; i < bmp->block_num; ++i)
    {
        if((bmp->bmp[i/8] & (1<<(i%8))) == 0)
        {
            bmp->bmp[i/8] |= (1<<(i%8));
            bmp->last_alloc_block = i;
            bmp->free_block_num--;
            bmp->used_block_num++;
            return (i+1);
        }
    }
    return -2; //no space
}
void * mmpool_fixed_bmp_addr(mmpool_strategy_fixed_bitmap_t * bmp, int id, int block_size)
{
    if(id <= 0 || bmp->used_block_num == 0 || id > bmp->block_num)
    {
        return NULL;//no space
    }
    int idx = id - 1;
    return (void*)((char*)bmp + bmp->data_offset_from_this + idx*block_size);
}
int    mmpool_fixed_bmp_addr(mmpool_strategy_fixed_bitmap_t * bmp, void * p, int block_size)
{    
    int idx = ((char*)p - (char*)bmp - bmp->data_offset_from_this)/block_size;
    if(idx < 0)
    {
        return -1;
    }
    return (idx+1);
}
int mmpool_fixed_bmp_free(mmpool_strategy_fixed_bitmap_t * bmp, int id)
{
    if(id <= 0 || bmp->used_block_num == 0 || id > bmp->block_num)
    {
        return -1;//no space
    }
    int idx = id - 1;
    if(bmp->bmp[idx/8] & (1 << (idx%8)) )
    {
        bmp->bmp[idx/8] &= ~(1<<(idx % 8));
        bmp->free_block_num++;
        bmp->used_block_num--;
        return 0;
    }
    return -2;   
}


//////////////////////////////////////////////////////////////////////////


static void mmpool_init(int strategy_type, void * buffer, int sz, int block_size, int block_num)
{
    mmpool_t * pool = (mmpool_t*)buffer;
    bzero(buffer,sz);
    pool->buffer_size = sz;
    pool->block_size = block_size;
    if(strategy_type == E_MMPOOL_TYPE_FIXED_BMP)
    {
        pool->block_size = block_size;
        pool->type = strategy_type;
        pool->magic = MMPOOL_MAGIC;
        pool->block_size = block_size;
        /////////////////////////////////
        pool->fixed_bmp.block_num = block_num;
        pool->fixed_bmp.free_block_num = block_num;
        int bmp_byte_count =  (block_num + 7) / 8;
        pool->fixed_bmp.data_offset_from_this= \
            sizeof(mmpool_strategy_fixed_bitmap_t) + bmp_byte_count - 1;
        //total - bmp->bmp + data_from_this - 
        pool->data_size = sz - ((char * )(&pool->fixed_bmp) - (char*)buffer + pool->fixed_bmp.data_offset_from_this);
    }
    pool->free_size = pool->data_size;

    
    
}


mmpool_t *   mmpool_create(void * buffer, int sz,
                        int strategy_type, int block_size, int block_num)
{
    if(block_num <= 0)
    {
        return NULL;
    }                        
    int total_size_min = mmpool_calc_size( block_num,  block_size,  strategy_type);
    if(total_size_min > sz)
    {
        return NULL;
    }
    mmpool_init(strategy_type,buffer,sz,block_size,block_num);
    return (mmpool_t*)buffer;
}
mmpool_t *   mmpool_attach(void * buffer, int sz)
{
    mmpool_t * pool = (mmpool_t*)buffer;
    if(pool->magic != MMPOOL_MAGIC)
    {
        return NULL;//error magic checking
    }
    if(sz < pool->buffer_size )
    {
        return NULL;
    }
    //todo check stratgy type
    return pool;    
}
size_t       mmpool_calc_size(int block_num, int block_size, int strategy_type )
{
    size_t total_size = sizeof(mmpool_t) + block_size * block_num;
    if(strategy_type == E_MMPOOL_TYPE_FIXED_BMP)
    {
        total_size += (block_num + 7) / 8;//bmp size
    }    
    return total_size;    
}
int          mmpool_destroy(mmpool_t * pool)
{
    //nothing todo
    return 0;
}


int          mmpool_alloc(mmpool_t * pool ,int block_num)//>0
{
    int id ;
    switch(pool->type)
    {
        case E_MMPOOL_TYPE_FIXED_BMP:
            block_num = 1;
            id = mmpool_fixed_bmp_alloc(&pool->fixed_bmp);
        default:
            return -1;
    }
    if(id > 0)
    {
        //alloc success
        pool->free_size -= pool->block_size * block_num;
        pool->used_size += pool->block_size * block_num;
    }
    return 0;
}
int          mmpool_free(mmpool_t * pool ,int id)
{
    int ret;
    switch(pool->type)
    {
        case E_MMPOOL_TYPE_FIXED_BMP:
            ret = mmpool_fixed_bmp_free(&pool->fixed_bmp, id);
        default:
            return -1;
    }
    if(ret == 0)
    {
        //alloc success
        pool->free_size += pool->block_size ;
        pool->used_size -= pool->block_size ;
    }
    return ret;
}
void *       mmpool_addr(mmpool_t * pool ,int id)
{
    switch(pool->type)
    {
        case E_MMPOOL_TYPE_FIXED_BMP:
            return mmpool_fixed_bmp_addr(&pool->fixed_bmp, id, pool->block_size);
        default:
            return NULL;
    }
}
int          mmpool_id(mmpool_t * pool ,void *  p)
{
    switch(pool->type)
    {
        case E_MMPOOL_TYPE_FIXED_BMP:
            return mmpool_fixed_bmp_addr(&pool->fixed_bmp, p, pool->block_size);
        default:
            return -1;
    }
}

//traverse
int          mmpool_total(mmpool_t * pool)
{
    return pool->data_size / pool->block_size;
}
int          mmpool_used(mmpool_t * pool)
{
    return pool->used_size / pool->block_size;
}
int          mmpool_begin(mmpool_t * pool)
{
    return 1;
}
int          mmpool_next(mmpool_t * pool, int id)
{
    return id+1;
}


////////////////////////////////////////////////////////////////////////////










