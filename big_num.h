#ifndef BIG_NUM_H_
#define BIG_NUM_H_
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/types.h>    // kfree kvmalloc
#include <linux/vmalloc.h>  // vfree
#define BASE_BITS 64
#define SWAP(_a, _b) ()
typedef unsigned __int128 dbase_t;
typedef u64 base_t;
extern void kvfree(const void *addr);
extern void *kvrealloc(void *p, size_t oldsize, size_t newsize, gfp_t flags);
typedef struct {
    base_t *block;
    size_t block_num;
    size_t true_block_num;
} big_num_t;

big_num_t *big_num_create(size_t, base_t);

void big_num_add(big_num_t *, big_num_t *, big_num_t *);

void big_num_sub(big_num_t *, big_num_t *, big_num_t *);

void big_num_mul(big_num_t *, big_num_t *, big_num_t *);

void big_num_lshift(big_num_t *, int);

void big_num_square(big_num_t *, big_num_t *);

char *big_num_to_string(big_num_t *);

void big_num_free(big_num_t *);

bool big_num_is_zero(big_num_t *);

void big_num_reset(big_num_t *);

void big_num_resize(big_num_t *, int);

void big_num_trim(big_num_t *);

void big_num_swap(big_num_t **, big_num_t **);
#endif /* BIG_NUM_H_*/