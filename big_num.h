#ifndef BIG_NUM_H_
#define BIG_NUM_H_
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/vmalloc.h>
extern void kvfree(const void *addr);
extern void *kvrealloc(void *p, size_t oldsize, size_t newsize, gfp_t flags);
typedef struct {
    u32 *block;
    size_t block_num;
} big_num_t;

big_num_t *big_num_create(size_t, u32);

big_num_t *big_num_add(big_num_t *, big_num_t *);

big_num_t *big_num_dup(big_num_t *);

char *big_num_to_string(big_num_t *);

void big_num_free(big_num_t *);
#endif /* BIG_NUM_H_*/