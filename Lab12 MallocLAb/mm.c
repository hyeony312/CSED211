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

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)
#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

// define macro
#define WSIZE 4 // word size
#define DSIZE 8 // double word size
#define CHUNKSIZE (1<<12) // 4KiB size of heap to extend

#define MAX(x,y) ((x)>(y)? (x) : (y)) // find bigger one

#define PACK(size,alloc) ((size)| (alloc)) // pack size and alloc bit in a block

// from address of p, read and write a word 
#define GET(p) (*(unsigned int*)(p)) // read a word
#define PUT(p,val) (*(unsigned int*)(p)=(val)) // write

// from address of p, read a size and allocate some feild
#define GET_SIZE(p) (GET(p) & ~0x7) // read size
#define GET_ALLOC(p) (GET(p) & 0x1) // read alloc

// from address of bp, compute position of header and footer
#define HDRP(bp) ((char*)(bp) - WSIZE)
#define FTRP(bp) ((char*)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

// from address of bp, compute position from next and previous block
#define NEXT_BLKP(bp) ((char*)(bp) + GET_SIZE(((char*)(bp) - WSIZE)))
#define PREV_BLKP(bp) ((char*)(bp) - GET_SIZE(((char*)(bp) - DSIZE)))

// Pointer
static char *heap_listp; //heap pointer
static char *next_p; //next pointer

// define functions
static void *extend_heap(size_t size);
static void *coalesce(void* bp);
static void *find_fit(size_t size);
static void place(void *bp, size_t asize);

/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    if((heap_listp = mem_sbrk(4*WSIZE)) == (void*)-1){
        return -1;
    }
    PUT(heap_listp,0);
    PUT(heap_listp + (1*WSIZE), PACK(DSIZE,1)); // make prologue header
    PUT(heap_listp + (2*WSIZE), PACK(DSIZE,1)); // make prologue footer
    PUT(heap_listp + (3*WSIZE), PACK(0,1)); // make epilogue block header
    heap_listp += (2*WSIZE); // move pointer between the header and footer 
    next_p = heap_listp;

    if (extend_heap(CHUNKSIZE/WSIZE)==NULL) { // extend heap
        return -1;
    }

    return 0;
}

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    size_t asize;   // Adjusted block size
    char *bp;       // Block pointer

    // Check for exception
    if (size == 0) {
        return NULL;
    }

    // Calculate the adjusted block
    if (size <= DSIZE) {
        // If the requested size is small, allocate a minimum block size
        asize = 2 * DSIZE;
    } else {
        // Align the block size to ensure proper alignment and add the header and footer sizes
        asize = ALIGN(size + DSIZE);
    }

    // Find a free block that best fits the adjusted block size
    bp = find_fit(asize);

    // If no suitable free block is found, extend the heap and allocate a new block
    if (bp == NULL) {
        if ((bp = extend_heap(asize / WSIZE)) == NULL) {
            // Return NULL if extending the heap fails
            return NULL;
        }
        // Place the block in the newly allocated space
        place(bp, asize);
    } else {
        // If a suitable free block is found, place the block in that space
        place(bp, asize);
    }

    // Return the pointer to the allocated block
    return bp;
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
    size_t size = GET_SIZE(HDRP(ptr));  // Get the size

    PUT(HDRP(ptr), PACK(size, 0));  // Set header for the free block
    PUT(FTRP(ptr), PACK(size, 0));  // Set footer for the free block

    // Coalesce the free block with adjacent free blocks
    coalesce(ptr);
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
    void *oldptr = ptr; // Store the old pointer
    void *newptr; // Declare a new pointer for the reallocated block
    size_t newSize = size + DSIZE; // Calculate the new size required (considering the overhead for headers and footers)
    size_t oldSize = GET_SIZE(HDRP(oldptr)); // Get the current size of the block pointed by oldptr 
    size_t addsize = oldSize; // Variable to store the cumulative size of contiguous free blocks
    int flag = 0;
    int flag_over = 0;

    // If the requested size is less than or equal to the current size, return the old pointer
    if (newSize <= oldSize) {
        return oldptr;
    }

    // Search for contiguous free blocks starting from the oldptr
    void *temp = oldptr;
    for (temp = oldptr; GET_ALLOC(HDRP(NEXT_BLKP(temp))) == 0; temp = NEXT_BLKP(temp)) {
        // Accumulate the size of contiguous free blocks
        addsize += GET_SIZE(HDRP(NEXT_BLKP(temp)));
        if(temp == next_p){
            flag_over = 1;
        }

        if(newSize <= addsize){ // Break if find sufficient free block
            flag = 1;
            break;
        }
    }

    // When size is sufficient
    if (flag) {
        PUT(HDRP(oldptr), PACK(addsize, 1));
        PUT(FTRP(oldptr), PACK(addsize, 1));
        if(flag_over==1){
            next_p = oldptr;
        }
        return oldptr;
    }
    else{
        // Allocate a new block with the requested size
    newptr = mm_malloc(newSize);
    if (newptr == NULL) {
        // If mm_malloc fails, return NULL
        return NULL;
    }

    // Copy the data from the old block to the new block
    memcpy(newptr, oldptr, newSize);
    mm_free(oldptr);
    }

    // Return the new pointer
    return newptr;
}

static void *extend_heap(size_t words){
    char *bp;
    size_t size;

    if((words%2)==0){ // when words size is even
        size = WSIZE*words;
    }
    else{ // case for odd
        size = WSIZE*(words+1);
    }

    if((bp = mem_sbrk(size))==(void*)-1){ // when there is an error extending heap, return NULL
        return NULL;
    }

    PUT(HDRP(bp), PACK(size, 0));   // free block header
    PUT(FTRP(bp), PACK(size, 0));   // free block footer
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0,1)); // new epilogue header
    
    return coalesce(bp);
}

static void* coalesce(void* bp)
{
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));

    if (prev_alloc && next_alloc) { /* Case 1: Both previous and next blocks are allocated */
    // No merging is required, return the current block pointer
    next_p=bp;
    return bp;
    }
    else if (prev_alloc && !next_alloc) { /* Case 2: Only the next block is free */
    // Merge the current block with the next free block
    size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    }
    else if (!prev_alloc && next_alloc) { /* Case 3: Only the previous block is free */
    // Merge the current block with the previous free block
    size += GET_SIZE(HDRP(PREV_BLKP(bp)));
    PUT(FTRP(bp), PACK(size, 0));
    PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
    // Update the block pointer to the start of the merged block
    bp = PREV_BLKP(bp);
    } 
    else { /* Case 4: Both previous and next blocks are free */
    // Merge the current block with both the previous and next free blocks
    size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(FTRP(NEXT_BLKP(bp)));
    PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
    PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
    // Update the block pointer to the start of the merged block
    bp = PREV_BLKP(bp);
    }

    next_p = bp;
    // Return the block pointer after potential merging
    return bp;
}

static void *find_fit(size_t size)
{
    // void *bp;      // Pointer to traverse the heap
    // void *best = NULL;  // Pointer to the best fit block found so far
    // int flag = 0;   // Flag to indicate whether a suitable block has been found

    // // Traverse the heap to find the best fit block
    // for (bp = heap_listp; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp)) {
    //     // Skip allocated blocks
    //     if (GET_ALLOC(HDRP(bp)))
    //         continue;

    //     // If no suitable block has been found yet
    //     if (flag == 0) {
    //         // Check if the current block is large enough
    //         if (size <= GET_SIZE(HDRP(bp))) {
    //             best = bp;  
    //             flag++;     // Suitable block has been found
    //         }
    //     }
    //     else {
    //         // Choose the block with the smallest size that still fits the requested size
    //         if ((GET_SIZE(HDRP(best)) > GET_SIZE(HDRP(bp))) && (size <= GET_SIZE(HDRP(bp)))) {
    //             best = bp;  // Update the best fit block
    //         }
    //     }
    // }

    // // Return the pointer to the best fit block or NULL
    // return best;

    void *bp; // Pointer to traverse the heap
    void *next = NULL;  // Pointer to the best fit block found so far

    if(next_p == NULL){
        next_p = heap_listp;
    }

    for (bp = next_p; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp)) {
        // Skip allocated blocks
        if (GET_ALLOC(HDRP(bp)))
            continue;

        if (size <= GET_SIZE(HDRP(bp))){
            next_p = bp;
            next = bp;
            break;
        }
    }

    return next;
}

static void place(void *bp, size_t size){
    size_t csize = GET_SIZE(HDRP(bp));  // Get the size of the free block

    // Check if there is enough space to split the block
    if ((csize - size) >= (2 * DSIZE)) {
        // Split the block
        PUT(HDRP(bp), PACK(size, 1));   // Set header
        PUT(FTRP(bp), PACK(size, 1));   // Set footer
        bp = NEXT_BLKP(bp);             // Move next block
        next_p = bp;
        PUT(HDRP(bp), PACK(csize - size, 0));  // Set header for remaining free block
        PUT(FTRP(bp), PACK(csize - size, 0));  // Set footer for remaining free block
    } else {
        next_p = bp;
        // Allocate the entire block
        PUT(HDRP(bp), PACK(csize, 1));   // Set header
        PUT(FTRP(bp), PACK(csize, 1));   // Set footer
    }
}

// // consistency checker

// static int mm_check(){
//     void *temp;
//     int flag = 0;

//     // Is every block in the free list marked as free?
//     for(temp=start_listp; GET_SIZE(HDRP(temp)) > 0; temp = NEXT_BLKP(temp)){
//         if(GET_ALLOC(temp) == 1) continue;
//         else if(GET_ALLOC(temp) == 0) continue;
//         else {
//             printf("Consistency error: block %p in free list but marked allocated!", temp);
//             return 1;
//         }
//     }

//     // Are there any contiguous free blocks that somehow escaped coalescing?
//     for(temp=start_listp; GET_SIZE(HDRP(temp)) > 0; temp = NEXT_BLKP(temp)){
//         if(GET_ALLOC(temp) == 0){
//             flag += 1;
//         }
//         else {
//             flag = 0;
//         }

//         if(flag >= 2){
//             printf("Consistency error: block %p missed coalescing!", temp);
//             return 1;
//         }
//     }

//     return 0;
// }







