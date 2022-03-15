#include "big_num.h"
big_num_t *big_num_add(big_num_t *a, big_num_t *b)
{
    if (!a || !b)
        return NULL;
    big_num_t *big, *small;
    big = a->block_num >= b->block_num ? a : b;
    small = a->block_num < b->block_num ? a : b;
    big_num_t *c = big_num_create(big->block_num, 0);
    if (!c)
        return NULL;
    u32 cy = 0;
    for (size_t i = 0; i < small->block_num; ++i) {
        c->block[i] = a->block[i] + b->block[i] + cy;
        cy = (u32)(((u64) a->block[i] + (u64) b->block[i]) >> 32);
    }
    for (size_t i = small->block_num; i < big->block_num; ++i) {
        c->block[i] = big->block[i] + cy;
        cy = (u32)(((u64) big->block[i] + cy) >> 32);
    }
    if (cy) {
        c->block = kvrealloc(c->block, sizeof(u32) * c->block_num,
                             sizeof(u32) * c->block_num + 1, GFP_KERNEL);
        if (!c->block) {
            kvfree(c);
            return NULL;
        }
        c->block_num += 1;
        c->block[c->block_num - 1] = cy;
    }
    return c;
}
big_num_t *big_num_dup(big_num_t *a)
{
    big_num_t *b = big_num_create(a->block_num, 0);
    for (size_t i = 0; i < a->block_num; ++i)
        b->block[i] = a->block[i];
    return b;
}
big_num_t *big_num_create(size_t num, u32 init)
{
    big_num_t *a = kvmalloc(sizeof(big_num_t), GFP_KERNEL);
    if (!a)
        return NULL;
    a->block = kvmalloc(sizeof(u32) * num, GFP_KERNEL);
    if (!a->block)
        return NULL;
    a->block_num = num;
    for (size_t i = 1; i < num; ++i)
        a->block[i] = 0;
    a->block[0] = init;
    return a;
}
char *big_num_to_string(big_num_t *a)
{
    size_t len = (a->block_num * sizeof(u32) * 8) / 3 + 2;
    char *ret = kvmalloc(len * sizeof(char), GFP_KERNEL);
    if (!ret) {
        big_num_free(a);
        return NULL;
    }

    for (size_t i = 0; i < len - 1; ++i) {
        ret[i] = '0';
    }

    ret[len - 1] = '\0';
    for (int i = a->block_num - 1; i >= 0; --i) {
        for (u32 m = 0x80000000; m; m >>= 1) {
            int cy = (a->block[i] & m) != 0;
            for (int j = len - 2; j >= 0; --j) {
                ret[j] = (ret[j] - '0') * 2 + cy + '0';
                cy = ret[j] > '9';
                if (cy)
                    ret[j] -= 10;
            }
        }
    }
    big_num_free(a);
    return ret;
}
void *kvrealloc(void *p, size_t oldsize, size_t newsize, gfp_t flags)
{
    if (oldsize >= newsize) {
        kvfree(p);
        return NULL;
    }
    void *newp = kvmalloc(newsize, GFP_KERNEL);
    if (!newp) {
        kvfree(p);
        return NULL;
    }
    memcpy(newp, p, oldsize);
    kvfree(p);
    return newp;
}

void kvfree(const void *addr)
{
    if (is_vmalloc_addr(addr))
        vfree(addr);
    else
        kfree(addr);
}
void big_num_free(big_num_t *a)
{
    if (!a)
        return;
    if (a->block)
        kvfree(a->block);
    kvfree(a);
}