/*
 * SylixOS(TM)  LW : long wing
 * Copyright All Rights Reserved
 *
 * MODULE PRIVATE DYNAMIC MEMORY IN VPROCESS PATCH
 * this file is support current module malloc/free use his own heap in a vprocess.
 * 
 * Author: Han.hui <sylixos@gmail.com>
 */

#ifndef __TLSF_MALLOC_H
#define __TLSF_MALLOC_H

#include "tlsf.h"

/*
 * tlsf lock
 */
void __vp_patch_lock(void);
void __vp_patch_unlock(void);

/*
 * tlsf handle
 */
#define TLSF_HANDLE(pheap)  ((pheap)->HEAP_pvStartAddress)

/*
 * update memory infomation
 */
#define VP_MEM_INFO(pheap)  \
        do {    \
            (pheap)->HEAP_stTotalByteSize = 0;  \
        } while (0)
        
/*
 * build a memory management
 */
#define VP_MEM_CTOR(pheap, mem, size)   \
        do {    \
            TLSF_HANDLE(pheap) = (tlsf_t)tlsf_create_with_pool(mem, size);  \
        } while (0)
        
/*
 * destory memory management
 */
#define VP_MEM_DTOR(pheap) tlsf_destroy((tlsf_t)TLSF_HANDLE(pheap))

/*
 * add memory to management
 */
#define VP_MEM_ADD(pheap, mem, size) tlsf_add_pool((tlsf_t)TLSF_HANDLE(pheap), mem, size)

/*
 * allocate memory
 */
static LW_INLINE void *VP_MEM_ALLOC (PLW_CLASS_HEAP  pheap, size_t nbytes)
{
    REGISTER void *ret;
    
    __vp_patch_lock();
    ret = tlsf_malloc((tlsf_t)TLSF_HANDLE(pheap), nbytes);
    __vp_patch_unlock();
    
    return  (ret);
}

/*
 * allocate memory align
 */
static LW_INLINE void *VP_MEM_ALLOC_ALIGN (PLW_CLASS_HEAP pheap, size_t nbytes, size_t align)
{
    REGISTER void *ret;
    
    __vp_patch_lock();
    ret = tlsf_memalign((tlsf_t)TLSF_HANDLE(pheap), align, nbytes);
    __vp_patch_unlock();
    
    return  (ret);
}


/*
 * re-allocate memory
 */
static LW_INLINE void *VP_MEM_REALLOC (PLW_CLASS_HEAP pheap, void *ptr, size_t new_size, int do_check)
{
    REGISTER void *ret;
    
    __vp_patch_lock();
    ret = tlsf_realloc((tlsf_t)TLSF_HANDLE(pheap), ptr, new_size);
    __vp_patch_unlock();
    
    return  (ret);
}

/*
 * free memory
 */
#define VP_MEM_FREE(pheap, ptr, do_check)   \
        do {    \
            __vp_patch_lock();  \
            tlsf_free((tlsf_t)TLSF_HANDLE(pheap), ptr);  \
            __vp_patch_unlock();    \
        } while (0)

#endif /* __TLSF_MALLOC_H */

/*
 * end
 */
