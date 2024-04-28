/* Tested by myself:
* amptjp-bal.rep      2027704   69177811
* binary-bal.rep      2095544   149653823
* binary2-bal.rep     1119880   317196967
* cccp-bal.rep        1685352   73218259
* coalescing-bal.rep  8208      196631156
* cp-decl-bal.rep     3187816   87139382
* expr-bal.rep        3434160   68678527
* random-bal.rep      16126952  63757061
* random2-bal.rep     15759408  62455792
* realloc-bal.rep     615448    232612198
* realloc2-bal.rep    32224     185049867
* short1-bal.rep      10264     90929
* short2-bal.rep      18368     89083
*/


#include "kernel/types.h"

//
#include "user/user.h"

//
#include "ummalloc.h"

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT - 1)) & ~0x7)



#define SIZE_T_SIZE (ALIGN(sizeof(uint)))


struct memblk
{
  uint data; //next ptr,the last bit shows if THIS memblk is free.
  uint size;
};

#define NEXTPTR(data) ((struct memblk *)((long long)(data) & ~0x7))

#define NOTFREE(data) ((long long)(data) & 0x1) 

#define SET_FREE(data) (uint)((long long)(data) & ~0x1)

#define SET_NOT_FREE(data) (uint)((long long)(data) | 0x1)


#define SIZE_HEADER (ALIGN(sizeof(struct memblk)))

struct memblk *first_memblk;

struct memblk *last_memblk;

//struct memblk * ptest;

void push(void *ptr, uint size)
{
  struct memblk *p = (struct memblk *)ptr;
  p->data = SET_NOT_FREE(0);
  p->size = size;
  if (last_memblk != 0)
  {
    last_memblk->data = (uint)((long long)p | NOTFREE(last_memblk->data));
    last_memblk = p;
  }
  else
  {
    first_memblk = p;
    last_memblk = p;
  }
}

// newsize must have been aligned; size-newsize >= 16
// split a block into 2 blocks:the formmer occupied;the latter free.Used in alloc and realloc
void *split(void *ptr, uint newsize)
{
  struct memblk *p = (struct memblk *)ptr;
  struct memblk *newp = (struct memblk *)((char*)ptr + newsize);
  newp->size = p->size - newsize;
  newp->data = SET_FREE(NEXTPTR(p->data));
  p->size = newsize;
  p->data = SET_NOT_FREE(newp);
  if(p == last_memblk) last_memblk = newp;
  return newp;
}

void merge(void *ptr)
{
  struct memblk *p = (struct memblk *)ptr;
  p->size = p->size + NEXTPTR(p->data)->size;
  p->data = NEXTPTR(p->data)->data;
  if(NEXTPTR(p->data) == 0) last_memblk = p;
}

void *find(uint size)
{
  for (struct memblk *p = first_memblk; p != 0; p = NEXTPTR(p->data))
  {
    while (NEXTPTR(p->data) != 0 && NOTFREE(p->data) == 0 && NOTFREE(NEXTPTR(p->data)->data) == 0){
      merge(p);
    }
    if (p->size >= size && NOTFREE(p->data) == 0){
      //printf("found p size:%d\n",p->size);
      return p;
    }
    
  }
  if (last_memblk != 0 && NOTFREE(last_memblk->data) == 0){
    sbrk(size - last_memblk->size);
    last_memblk->size = size;
    //printf("found p size:%d\n",last_memblk->size);
    return last_memblk;
  } 
  return 0;
}

void *find2(uint size){
  return 0;
}

/*
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
  first_memblk = 0;
  last_memblk = 0;
  //printf("%d\n",sizeof(struct memblk));
  //printf("%d\n",SIZE_HEADER);

  return 0;
}

/*
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(uint size)
{
  uint newsize = ALIGN(size + SIZE_HEADER);
  void *p = find(newsize);
  if (p == 0)
  {
    p = sbrk(newsize);
    if (p == (void *)-1)
      return 0;
    else
    {
      push(p,newsize);
      return (void *)((char *)p + SIZE_HEADER);
    }
  }else{
    struct memblk* ptr = p;
    ptr->data = SET_NOT_FREE(ptr->data);
    if(ptr->size >= SIZE_HEADER + newsize) split(p,newsize);
    //printf("allocated:%d\n",ptr->size);
    return (void *)((char *)p + SIZE_HEADER);
  }
  
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr) {
  struct memblk* p =(struct memblk*)((char *)ptr-SIZE_HEADER);
  p->data = SET_FREE(p->data);
}


void *mm_realloc(void *ptr, uint size)
{
  void *oldptr = ptr;
  void *newptr;
  uint copySize;
  //printf("realloc!\n");
  if (size == 0) {
    mm_free(ptr);
    return 0;
  }
  struct memblk* old_header = (struct memblk*)((char *)oldptr - SIZE_HEADER);
  if(old_header == last_memblk){
    if(size <= old_header->size - SIZE_HEADER){
      return oldptr;
    }
    else{
      sbrk(ALIGN(size)-old_header->size + SIZE_HEADER);
      old_header->size = ALIGN(size) + SIZE_HEADER;
      return oldptr;
    }
  }
  newptr = mm_malloc(size);
  if (newptr == 0) return 0;
  copySize = old_header->size - SIZE_HEADER;
  if (size < copySize) copySize = size;
  memcpy(newptr, oldptr, copySize);
  mm_free(oldptr);
  return newptr;
}
//