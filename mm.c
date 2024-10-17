/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 * 
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "ateam",
    /* First member's full name */
    "Harry Bovik",
    /* First member's email address */
    "bovik@cs.cmu.edu",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""
};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)


#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

extern char* mem_brk;
#define CHUNK_SIZE (1 << 10)
#define WORD 4

#define MAKE_HEADER(size, alloc) ((size) | (alloc))
#define GET_WORD(ptr) (*(unsigned int*)(ptr))
#define PUT_WORD(ptr, val) (*(unsigned int*)(ptr) = (val))
#define GET_SIZE(ptr) (GET_WORD(ptr) & ~0x7)
#define IS_EMPTY(ptr) (!(GET_WORD(ptr) & 0b1))
#define NEXT_BLOCK(ptr) ((ptr) + (GET_SIZE(ptr)))

typedef char* pointer;
typedef unsigned int header;

const header epilog = MAKE_HEADER(0, 1);

static char* heap_first;
static char* heap_last;

static void* expend_heap(size_t words);
static void* coalesce(void* ptr);

void malloc_test();
void malloc_test() {

    printf("\n********************************************************\n\n");

    printf("heap first: %p\n", heap_first);
    printf("heap last: %p\n", heap_last);

    char* ptr = heap_first;
    int block_cnt = 0;
    unsigned long long int f = 0;
    while(0 < GET_SIZE(ptr + f) && ptr + f < heap_last) {
        printf("block %d\nis allocate: %d\nsize: %d\n", block_cnt++, !IS_EMPTY(ptr + f), GET_SIZE(ptr + f));
        printf("HEADER: %p, %0.4o\n", ptr + f, GET_WORD(ptr + f));
        //printf("size: %d\n", GET_SIZE(ptr + f));
        f = f + GET_SIZE(ptr + f) - WORD;
        printf("FOOTER: %p, %0.4o\n", ptr + f, GET_WORD(ptr + f));

        printf("\n");
        f += WORD;
    }

    printf("\n********************************************************\n");
    return;
}

/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    // init heap
    if((heap_first = mem_sbrk(3 * WORD)) == (void*)-1)
        return -1;

    PUT_WORD(heap_first, 0);
    heap_first += WORD;
    
    header prolog_header = MAKE_HEADER(2 * WORD, 1);
    PUT_WORD(heap_first, prolog_header);  // prologue header
    PUT_WORD(heap_first + WORD, prolog_header);  // prologue footer
    heap_last = heap_first + 2 * WORD;

    if(expend_heap(CHUNK_SIZE) == NULL)
        return -1;
    
    return 0;
}

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
// void *mm_malloc(size_t size)
// {
// printf("malloc %d\n\n", size);
//     int newsize = ALIGN(size + SIZE_T_SIZE);
//     void *p = mem_sbrk(newsize);
//     if (p == (void *)-1)
// 	return NULL;
//     else {
//         *(size_t *)p = size;
//         return (void *)((char *)p + SIZE_T_SIZE);
//     }
// malloc_test();
// }
void *mm_malloc(size_t size)
{
// printf("\n\nmalloc %d\n\n", size);
// malloc_test();
// int k;
// scanf("%d", &k);
    if(size == 0)
        return NULL;

    // calculate block size
    size = size % ALIGNMENT ? ((size / ALIGNMENT) + 1) * ALIGNMENT : size;
    size += 2 * WORD;

    // find empty block
    pointer alloc_ptr = heap_first;
    size_t alloc_size = GET_SIZE(heap_first);

    pointer idx_ptr = heap_first;
    while(idx_ptr < heap_last) {
        size_t temp_size = GET_SIZE(idx_ptr);
        if(IS_EMPTY(idx_ptr))
            if(size <= temp_size && temp_size < alloc_ptr) {
                alloc_ptr = idx_ptr;
                alloc_size = temp_size;
            }

        idx_ptr += temp_size;
    }

    // if no appropriate empty block
    if(alloc_ptr == heap_first)
        if((alloc_ptr = expend_heap(size)) == NULL)
            return NULL;
    
    // allocate new block
    size_t old_size = GET_SIZE(alloc_ptr);
    header alloc_header = MAKE_HEADER(size, 1);
    PUT_WORD(alloc_ptr, alloc_header);
    PUT_WORD(alloc_ptr + size - WORD, alloc_header);

    // new empty block
    pointer empty_ptr = alloc_ptr + size;
    size_t empty_size = old_size - size;
    if(0 < empty_size) {
        header empty_header = MAKE_HEADER(empty_size, 0);
        PUT_WORD(empty_ptr, empty_header);
        PUT_WORD(empty_ptr + empty_size - WORD, empty_header);
    }
//printf("\nmalloc end\n\n");
// malloc_test();
    return alloc_ptr + WORD;
}

// /*
//  * mm_free - Freeing a block does nothing.
//  */
void mm_free(void *ptr)
{
    ptr -= WORD;
    if(IS_EMPTY(ptr))
        return;
    
    size_t free_size = GET_SIZE(ptr);
    header free_header = MAKE_HEADER(free_size, 0);
    PUT_WORD(ptr, free_header);
    
    coalesce(ptr);

    return;
}

static void* expend_heap(size_t size) {
// printf("\nexpend_heap %d\n\n", size);
// malloc_test();
// int k;
// scanf("%d", &k);
    // calculate chunk size
    int chunk_num;
    chunk_num = CHUNK_SIZE > size ? 1 : size / CHUNK_SIZE + 1;

    char* chunk_ptr;
    if((chunk_ptr = mem_sbrk(chunk_num * CHUNK_SIZE)) == (void*)-1)
        return NULL;

    heap_last += chunk_num * CHUNK_SIZE;

    // allocate empty block in new chunk
    header chunk_header = MAKE_HEADER(CHUNK_SIZE * chunk_num, 0);
    PUT_WORD(chunk_ptr, chunk_header);
    PUT_WORD(chunk_ptr + GET_SIZE(chunk_ptr) - WORD, chunk_header);
    
    return coalesce(chunk_ptr);
}

static void* coalesce(void* ptr) {
// printf("\ncoalesce %p\n\n", ptr);
// malloc_test();
// int k;
// scanf("%d", &k);
    pointer current_header = ptr;
    pointer predecessor_footer = ptr - WORD;
    pointer successor_header = ptr + GET_SIZE(ptr);

    size_t size = GET_SIZE(current_header);
    pointer ret_pointer = current_header;

    if(successor_header < heap_last && IS_EMPTY(successor_header))
        size += GET_SIZE(successor_header);

    if(IS_EMPTY(predecessor_footer)) {
        size += GET_SIZE(predecessor_footer);
        ret_pointer -= GET_SIZE(predecessor_footer);
    }

    header new_header = MAKE_HEADER(size, 0);
    PUT_WORD(ret_pointer, new_header);
    PUT_WORD(ret_pointer + size - WORD, new_header);

    return ret_pointer;
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
    void *oldptr = ptr;
    void *newptr;
    size_t copySize;
    
    newptr = mm_malloc(size);
    if (newptr == NULL)
      return NULL;
    copySize = *(size_t *)((char *)oldptr - SIZE_T_SIZE);
    if (size < copySize)
      copySize = size;
    memcpy(newptr, oldptr, copySize);
    mm_free(oldptr);
    return newptr;
}














