#include "big_num.h"
// c = a + b
void big_num_add(big_num_t *c, big_num_t *a, big_num_t *b)
{
    if (!a || !b)
        return;
    big_num_t *big, *small;
    big = a->block_num >= b->block_num ? a : b;
    small = a->block_num < b->block_num ? a : b;
    big_num_resize(c, big->block_num);
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
        big_num_resize(c, big->block_num + 1);
        c->block[c->block_num - 1] = cy;
    }
}

// c = a - b, assume a > b
void big_num_sub(big_num_t *c, big_num_t *a, big_num_t *b)
{
    if (!a || !b)
        return;
    big_num_resize(b, a->block_num);
    big_num_resize(c, a->block_num);
    u32 cy = 1;
    for (size_t i = 0; i < a->block_num; ++i) {
        u32 t = ~b->block[i];
        c->block[i] = a->block[i] + t + cy;
        cy = (u32)(((u64) a->block[i] + (u64) t) >> 32);
    }
}
void big_num_lshift(big_num_t *a, int k)
{
    u32 cy = 0;
    for (size_t i = 0; i < a->block_num; ++i) {
        u64 t = ((u64) a->block[i] << k) + cy;
        cy = (u32)(t >> 32);
        a->block[i] = (u32) t;
    }
    if (cy) {
        big_num_resize(a, a->block_num + 1);
        a->block[a->block_num - 1] = cy;
    }
}

void big_num_mul(big_num_t *c, big_num_t *a, big_num_t *b)
{
    if (!a || !b)
        return;
    big_num_reset(c);
    big_num_resize(c, a->block_num + b->block_num);
    if (big_num_is_zero(a) || big_num_is_zero(b)) {
        big_num_trim(c);
        return;
    }
    if (!c)
        return;
    for (size_t shift = 0; shift < b->block_num; ++shift) {
        u32 cy = 0;
        size_t i = 0;
        for (; i < a->block_num; ++i) {
            u64 t1 = (u64) a->block[i] * (u64) b->block[shift] + cy;
            cy = (u32)(t1 >> 32);
            u64 t2 = ((u64) c->block[i + shift]) + (u32) t1;
            cy += (u32)(t2 >> 32);
            c->block[i + shift] = (u32) t2;
        }
        for (int j = 0; cy != 0; ++j) {
            u64 t = (u64) c->block[i + j + shift] + (u64) cy;
            c->block[i + j + shift] = (u32) t;
            cy = (u32)(t >> 32);
        }
    }
    big_num_trim(c);
}

void big_num_square(big_num_t *c, big_num_t *a)
{
    if (!a)
        return;
    big_num_resize(c, 2 * a->block_num);
    big_num_reset(c);
    if (big_num_is_zero(a)) {
        big_num_trim(c);
        return;
    }

    if (!c)
        return;
    for (size_t shift = 0; shift < a->block_num; ++shift) {
        u32 cy = 0;
        size_t i = shift + 1;
        for (; i < a->block_num; ++i) {
            u64 t1 = (u64) a->block[i] * (u64) a->block[shift] + cy;
            cy = (u32)(t1 >> 32);
            u64 t2 = ((u64) c->block[i + shift]) + (u32) t1;
            cy += (u32)(t2 >> 32);
            c->block[i + shift] = (u32) t2;
        }
        for (int j = 0; cy != 0; ++j) {
            u64 t = (u64) c->block[i + j + shift] + (u64) cy;
            c->block[i + j + shift] = (u32) t;
            cy = (u32)(t >> 32);
        }
    }
    big_num_lshift(c, 1);
    u32 cy = 0;
    for (size_t shift = 0; shift < a->block_num; ++shift) {
        u64 t1 = (u64) a->block[shift] * (u64) a->block[shift] + cy;
        cy = (u32)(t1 >> 32);
        u64 t2 = ((u64) c->block[2 * shift]) + (u32) t1;
        cy += (u32)(t2 >> 32);
        c->block[2 * shift] = (u32) t2;
        for (int j = 1; cy != 0; ++j) {
            u64 t = (u64) c->block[j + 2 * shift] + (u64) cy;
            c->block[j + 2 * shift] = (u32) t;
            cy = (u32)(t >> 32);
        }
    }
    big_num_trim(c);
}

void big_num_cpy(big_num_t *c, big_num_t *a)
{
    if (!a)
        return;
    big_num_resize(c, a->block_num);
    memcpy(c->block, a->block, sizeof(u32) * a->block_num);
}

big_num_t *big_num_create(size_t num, u32 init)
{
    big_num_t *a = kvmalloc(sizeof(big_num_t), GFP_KERNEL);
    if (!a)
        return NULL;
    a->block = kvmalloc(sizeof(u32) * num, GFP_KERNEL);
    if (!a->block) {
        kvfree(a);
        return NULL;
    }
    a->block_num = num;
    memset(a->block, 0, sizeof(u32) * num);
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
        void *newp = kvmalloc(newsize, GFP_KERNEL);
        if (!newp) {
            kvfree(p);
            return NULL;
        }
        memcpy(newp, p, newsize);
        kvfree(p);
        return newp;
    } else {
        void *newp = kvmalloc(newsize, GFP_KERNEL);
        if (!newp) {
            kvfree(p);
            return NULL;
        }
        memcpy(newp, p, oldsize);
        kvfree(p);
        return newp;
    }
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

void big_num_reset(big_num_t *a)
{
    if (!a || !a->block)
        return;
    memset(a->block, 0, sizeof(u32) * a->block_num);
}
void big_num_resize(big_num_t *a, int num)
{
    if (a->block_num == num)
        return;
    a->block = kvrealloc(a->block, sizeof(u32) * a->block_num,
                         sizeof(u32) * num, GFP_KERNEL);
    if (a->block_num > num) {
        a->block_num = num;
    } else {
        while (a->block_num < num)
            a->block_num++;
        a->block[a->block_num - 1] = 0;
    }
}
bool big_num_is_zero(big_num_t *a)
{
    for (size_t i = 0; i < a->block_num; ++i) {
        if (a->block[i] != 0)
            return false;
    }
    return true;
}
void big_num_trim(big_num_t *a)
{
    size_t num = a->block_num - 1;
    while (a->block[num] == 0 && num != 0)
        num--;
    big_num_resize(a, num + 1);
}