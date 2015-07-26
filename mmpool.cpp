#include "mmpool.h"
#include <strings.h>    
#include <stdio.h>

#pragma pack(1)
#define     MMPOOL_MAGIC        (0x1234)
typedef struct mmpool_strategy_fixed_bitmap_t
{
    int         last_alloc_block;
    int         total_block_num;
    int         free_block_num;
    int         used_block_num;
    int         max_block_idx;
    int         data_offset_from_this;
    unsigned    char  bmp[1];
}
mmpool_strategy_fixed_bitmap_t;

typedef struct mmpool_strategy_fixed_block_list_entry_t
{
    int         busy    :1;//free:0 or busy:1
    int         prev_idx :31;
    int         next_idx;
    unsigned char data[1];
}
mmpool_strategy_fixed_block_list_entry_t;

typedef struct mmpool_strategy_fixed_block_list_t
{
    int         total_block_num;
    int         used_block_num;
    int         free_block_num;
    int         entry_size;    
    int         used_block_head;
    int         free_block_head;    
}
mmpool_strategy_fixed_block_list_t;


typedef struct mmpool_base_t
{
    int    magic;
    int    type;
    int    block_size;
    size_t buffer_size;
    size_t free_size;
    size_t used_size;
}
mmpool_base_t ;

typedef struct mmpool_t
{
    mmpool_base_t   base;
    union
    {
        mmpool_strategy_fixed_bitmap_t        fixed_bmp;
        mmpool_strategy_fixed_block_list_t    fixed_block_list;
        //mmpool_strategy_flex_block_list_t     flex_block_list;
        //mmpool_strategy_flex_buddy_system_t   flex_buddy_system;        
    };
} mmpool_t;
#pragma pack()

//need optimal bit algorithm later 
int mmpool_fixed_bmp_alloc(mmpool_strategy_fixed_bitmap_t * bmp)
{
    if(bmp->used_block_num >= bmp->total_block_num)
    {        
        return -1;//error full
    }
    //todo optimal with bit
    for(int i = bmp->last_alloc_block + 1; i < bmp->total_block_num; ++i)
    {
        if((bmp->bmp[i/8] & (1<<(i%8))) == 0)
        {
            bmp->bmp[i/8] |= (1<<(i%8));
            bmp->last_alloc_block = i;
            bmp->free_block_num--;
            bmp->used_block_num++;
            if( i > bmp->max_block_idx)
            {
                bmp->max_block_idx = i;
            }
            return (i+1);
        }
    }
    //
    for(int i = 0;i <= bmp->last_alloc_block; ++i)
    {
        if((bmp->bmp[i/8] & (1<<(i%8))) == 0)
        {
            bmp->bmp[i/8] |= (1<<(i%8));
            bmp->last_alloc_block = i;
            bmp->free_block_num--;
            bmp->used_block_num++;
            if( i > bmp->max_block_idx)
            {
                bmp->max_block_idx = i;
            }
            return (i+1);
        }
    }
        
    return -2; //no space
}
void * mmpool_fixed_bmp_addr(mmpool_strategy_fixed_bitmap_t * bmp, int id, int block_size)
{
    if(id <= 0 || bmp->used_block_num == 0 || id > bmp->total_block_num)
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
    if(id <= 0 || bmp->used_block_num == 0 || id > bmp->total_block_num)
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
void mmpool_fixed_bmp_init(mmpool_strategy_fixed_bitmap_t * bmp, int block_num)
{
    /////////////////////////////////
    bmp->total_block_num = block_num;
    bmp->free_block_num = block_num;
    bmp->last_alloc_block = -1;
    int bmp_byte_count =  (block_num + 7) / 8;
    bmp->data_offset_from_this= \
        sizeof(mmpool_strategy_fixed_bitmap_t) + bmp_byte_count - 1;
}
int mmpool_fixed_bmp_next(mmpool_strategy_fixed_bitmap_t * bmp, int id)
{
    if(bmp->used_block_num <= 0 )
    {
        //invalid
        return 0;
    }
    for(int i = id;i <= bmp->max_block_idx; ++i)
    {
        if(bmp->bmp[i/8] & (1<<(i%8)))
        {
            return (i+1);
        }
    }
    return 0;
}
int mmpool_fixed_bmp_begin(mmpool_strategy_fixed_bitmap_t * bmp)
{
    return mmpool_fixed_bmp_next(bmp, 0);
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define get_block_list_entry(blocklist)  ((mmpool_strategy_fixed_block_list_entry_t *)((char*)blocklist + sizeof(*blocklist)) )
void mmpool_fixed_block_list_init(mmpool_strategy_fixed_block_list_t * blocklist, int block_size, size_t surplus_size )
{
    size_t available_size = surplus_size - sizeof(mmpool_strategy_fixed_block_list_t);

    int block_entry_size = block_size + sizeof(mmpool_strategy_fixed_block_list_entry_t);
    blocklist->free_block_num = available_size / block_entry_size;
    blocklist->entry_size = block_entry_size;
    blocklist->free_block_head = 0;
    blocklist->used_block_head = -1;

    mmpool_strategy_fixed_block_list_entry_t * pstblocklist = get_block_list_entry(blocklist);
    //init link list
    for(int i = 0 ;i < blocklist->free_block_num; ++i)
    {
        pstblocklist[i].prev_idx = i - 1;
        if(i + 1 != blocklist->free_block_num)
        {
            pstblocklist[i].next_idx = i + 1;        
        }
        else
        {
            pstblocklist[i].next_idx = -1;
        }
    }
    
}
int mmpool_fixed_block_list_alloc(mmpool_strategy_fixed_block_list_t * blocklist)
{
    mmpool_strategy_fixed_block_list_entry_t * pstblocklist = get_block_list_entry(blocklist);
    //free
    if(blocklist->free_block_head < 0 || blocklist->free_block_num <= 0)
    {
        return -1;
    }

    //erase from free
    int idx = blocklist->free_block_head;
    int next = pstblocklist[idx].next_idx;
    if(next >= 0)
    {
        pstblocklist[next].prev_idx = pstblocklist[idx].prev_idx;
    }
    if(pstblocklist[idx].prev_idx >= 0)
    {
        pstblocklist[pstblocklist[idx].prev_idx].next_idx = next;
    }
    else
    {
        blocklist->free_block_head = next;
    }
    //link to used
    pstblocklist[idx].busy = 1;
    pstblocklist[idx].prev_idx = -1; //prev
    if(blocklist->used_block_head >= 0)
    {
        pstblocklist[blocklist->used_block_head].prev_idx = idx;
    }
    pstblocklist[idx].next_idx = blocklist->used_block_head; //next    
    blocklist->used_block_head = idx;
    

    blocklist->free_block_num--;
    blocklist->used_block_num++;
    return idx+1;
}
int mmpool_fixed_block_list_free(mmpool_strategy_fixed_block_list_t * blocklist, int id)
{
    mmpool_strategy_fixed_block_list_entry_t * pstblocklist = get_block_list_entry(blocklist);
    //free
    if(id <= 0 || id > blocklist->total_block_num )
    {
        return -1;
    }

    int idx = id - 1;
    if(pstblocklist[idx].busy != 1)
    {
        //state error
        return -2;
    }

    pstblocklist[idx].busy = 0;
    //erase from used
    int next = pstblocklist[idx].next_idx;
    if(next >= 0)
    {
        pstblocklist[next].prev_idx = pstblocklist[idx].prev_idx;
    }
    if(pstblocklist[idx].prev_idx >= 0)
    {
        pstblocklist[pstblocklist[idx].prev_idx].next_idx = next;
    }
    else
    {
        //used head
        blocklist->used_block_head = next;
    }
    ////////////////////////////////////////////////////////////////    
    //link to free
    pstblocklist[idx].prev_idx = -1; //prev
    pstblocklist[idx].next_idx = blocklist->free_block_head; //next    
    if(blocklist->free_block_head >= 0)
    {
        pstblocklist[blocklist->free_block_head].prev_idx = idx;
    }
    blocklist->free_block_head = idx;

    blocklist->free_block_num++;
    blocklist->used_block_num--;
    return 0;
}

void * mmpool_fixed_block_list_addr(mmpool_strategy_fixed_block_list_t * blocklist, int id)
{
    mmpool_strategy_fixed_block_list_entry_t * pstblocklist = get_block_list_entry(blocklist);
    //free
    if(id <= 0 || id > blocklist->total_block_num )
    {
        return NULL;
    }    
    return (void*)&(pstblocklist[id-1].data[0]);
}

int mmpool_fixed_block_list_id(mmpool_strategy_fixed_block_list_t * blocklist, void * p)
{
    mmpool_strategy_fixed_block_list_entry_t * pstblocklist = get_block_list_entry(blocklist);
    if(p < pstblocklist )
    {
        return -1;
    }
    size_t offset = (char*)p - (char*)pstblocklist;
    int idx = offset / blocklist->entry_size;
    return (idx+1);
}

int mmpool_fixed_block_list_begin(mmpool_strategy_fixed_block_list_t * blocklist)
{
    return blocklist->used_block_head;
}
int mmpool_fixed_block_list_next(mmpool_strategy_fixed_block_list_t * blocklist, int id)
{
    mmpool_strategy_fixed_block_list_entry_t * pstblocklist = get_block_list_entry(blocklist);
    if(id <= 0 || id > blocklist->total_block_num)
    {
        return -1;
    }
    return  (pstblocklist[id-1].next_idx + 1);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void mmpool_init(int strategy_type, void * buffer,size_t sz, int block_size, int block_num)
{
    mmpool_t * pool = (mmpool_t*)buffer;
    bzero(buffer,sz);
    pool->base.buffer_size = sz;
    pool->base.block_size = block_size;
    pool->base.magic = MMPOOL_MAGIC;
    pool->base.used_size = sizeof(mmpool_base_t);
    pool->base.free_size = sz - sizeof(mmpool_base_t);
    pool->base.type = strategy_type;
    switch(strategy_type)
    {
        case E_MMPOOL_TYPE_FIXED_BMP:
            mmpool_fixed_bmp_init(&pool->fixed_bmp,block_num);
            pool->base.used_size += pool->fixed_bmp.data_offset_from_this;
            pool->base.free_size = sz - pool->base.used_size;
        break;
        case E_MMPOOL_TYPE_FIXED_BLOCK_LIST:
            mmpool_fixed_block_list_init(&pool->fixed_block_list, block_size, sz - sizeof(mmpool_base_t));
            pool->base.used_size += sizeof(pool->fixed_block_list);
            pool->base.free_size -= sizeof(pool->fixed_block_list);
        break;
        default:
        break;
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////////

mmpool_t *   mmpool_create(void * buffer, size_t sz,
                        int strategy_type, int block_size, int block_num)
{
    if(block_num <= 0)
    {
        return NULL;
    }
    size_t total_size_min = mmpool_calc_size( block_num,  block_size,  strategy_type);
    if(total_size_min > sz)
    {
        return NULL;
    }
    mmpool_init(strategy_type,buffer,sz,block_size,block_num);
    return (mmpool_t*)buffer;
}
mmpool_t *   mmpool_attach(void * buffer, size_t sz)
{
    mmpool_t * pool = (mmpool_t*)buffer;
    if(pool->base.magic != MMPOOL_MAGIC)
    {
        return NULL;//error magic checking
    }
    if(sz < pool->base.buffer_size )
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
    else
    if(strategy_type == E_MMPOOL_TYPE_FIXED_BLOCK_LIST)
    {
        total_size += (block_size + sizeof(mmpool_strategy_fixed_block_list_entry_t)) * block_num +
                        sizeof(mmpool_strategy_fixed_block_list_t);
    }
    //todo align with 8bytes
    return total_size;
}
int          mmpool_destroy(mmpool_t * pool)
{
    //nothing todo
    return 0;
}


int          mmpool_alloc(mmpool_t * pool )//>0
{
    int id = 0;
    switch(pool->base.type)
    {
        case E_MMPOOL_TYPE_FIXED_BMP:
            id = mmpool_fixed_bmp_alloc(&pool->fixed_bmp);
            break;
        case E_MMPOOL_TYPE_FIXED_BLOCK_LIST:
            id = mmpool_fixed_block_list_alloc(&pool->fixed_block_list);
            break;
        default:
            return -1;
    }
    if(id > 0)
    {
        //alloc success
        pool->base.free_size -= pool->base.block_size ;
        pool->base.used_size += pool->base.block_size ;
    }
    return id;
}
int          mmpool_free(mmpool_t * pool ,int id)
{
    int ret;
    switch(pool->base.type)
    {
        case E_MMPOOL_TYPE_FIXED_BMP:
            ret = mmpool_fixed_bmp_free(&pool->fixed_bmp, id);
            break;
        case E_MMPOOL_TYPE_FIXED_BLOCK_LIST:
            ret = mmpool_fixed_block_list_free(&pool->fixed_block_list, id);
            break;
        default:
            return -1;
    }
    if(ret == 0)
    {
        //alloc success
        pool->base.free_size += pool->base.block_size ;
        pool->base.used_size -= pool->base.block_size ;
    }
    return ret;
}
void *       mmpool_addr(mmpool_t * pool ,int id)
{
    switch(pool->base.type)
    {
        case E_MMPOOL_TYPE_FIXED_BMP:
            return mmpool_fixed_bmp_addr(&pool->fixed_bmp, id, pool->base.block_size);
        case E_MMPOOL_TYPE_FIXED_BLOCK_LIST:
            return mmpool_fixed_block_list_addr(&pool->fixed_block_list, id);
        default:
            return NULL;
    }
}
int          mmpool_id(mmpool_t * pool ,void *  p)
{
    switch(pool->base.type)
    {
        case E_MMPOOL_TYPE_FIXED_BMP:
            return mmpool_fixed_bmp_addr(&pool->fixed_bmp, p, pool->base.block_size);
        case E_MMPOOL_TYPE_FIXED_BLOCK_LIST:
            return mmpool_fixed_block_list_id(&pool->fixed_block_list, p);
        default:
            return -1;
    }
}

//traverse
size_t      mmpool_stat_free(mmpool_t * pool)
{
    return pool->base.free_size;
}
size_t      mmpool_stat_used(mmpool_t * pool)
{
    return pool->base.used_size ;
}
int          mmpool_begin(mmpool_t * pool)
{
    switch(pool->base.type)
    {
        case E_MMPOOL_TYPE_FIXED_BMP:
            return mmpool_fixed_bmp_begin(&pool->fixed_bmp);
        case E_MMPOOL_TYPE_FIXED_BLOCK_LIST:
            return mmpool_fixed_block_list_begin(&pool->fixed_block_list);
        default:
            return 0;
    }
}
int          mmpool_next(mmpool_t * pool, int id)
{    
    switch(pool->base.type)
    {
        case E_MMPOOL_TYPE_FIXED_BMP:
            return mmpool_fixed_bmp_next(&pool->fixed_bmp, id);
        case E_MMPOOL_TYPE_FIXED_BLOCK_LIST:
            return mmpool_fixed_block_list_next(&pool->fixed_block_list, id);
        default:
            return 0;
    }
    return 0;
}


////////////////////////////////////////////////////////////////////////////










