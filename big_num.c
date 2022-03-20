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
    big_num_reset(c);
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
    big_num_t *d = big_num_2comp(b);
    big_num_resize(c, a->block_num);
    big_num_reset(c);
    u32 cy = 0;
    for (size_t i = 0; i < a->block_num; ++i) {
        c->block[i] = a->block[i] + d->block[i] + cy;
        cy = (u32)(((u64) a->block[i] + (u64) d->block[i]) >> 32);
    }
    big_num_free(d);
}

big_num_t *big_num_2comp(big_num_t *a)
{
    big_num_t *a2 = big_num_dup(a);
    big_num_t *b = big_num_create(1, 1);
    for (size_t i = 0; i < a2->block_num; ++i) {
        a2->block[i] = ~a2->block[i];
    }
    big_num_t *c = big_num_create(1, 0);
    big_num_add(c, a2, b);
    big_num_free(b);
    big_num_free(a2);
    return c;
}

void big_num_lshift(big_num_t *a, int shift)
{
    if (big_num_is_zero(a))
        return;
    int cnt = 0, k = shift;
    for (; k > 32; k -= 32)
        cnt++;
    for (; cnt > 0; --cnt) {
        big_num_resize(a, a->block_num + 1);
        for (int i = a->block_num - 1; i > 0; --i) {
            a->block[i] = a->block[i - 1];
        }
        a->block[0] = 0;
    }
    u32 prev = 0;
    for (size_t i = 0; i < a->block_num; ++i) {
        u64 tmp = ((u64) a->block[i]) << k;
        u32 curr = (u32) tmp;
        a->block[i] = curr | prev;
        prev = (u32)(tmp >> 32);
    }
    if (prev) {
        big_num_resize(a, a->block_num + 1);
        a->block[a->block_num - 1] = prev;
    }
}

void big_num_mul(big_num_t *c, big_num_t *a, big_num_t *b)
{
    if (!a || !b)
        return;
    big_num_reset(c);
    if (big_num_is_zero(a) || big_num_is_zero(b)) {
        return;
    }
    if (!c)
        return;
    big_num_t *b2 = big_num_dup(b);
    int cnt = 0;
    for (size_t i = 0; i < a->block_num; ++i) {
        for (int k = 0; k < 32; ++k) {
            u32 m = 1u << k;
            if (a->block[i] & m) {
                big_num_lshift(b2, cnt);
                big_num_mul_add(c, b2);
                cnt = 0;
            }
            ++cnt;
        }
    }
    big_num_free(b2);
}

void big_num_mul_add(big_num_t *c, big_num_t *a)
{
    if (!a)
        return;
    if (big_num_is_zero(a)) {
        return;
    }
    if (a->block_num > c->block_num)
        big_num_resize(c, a->block_num);

    u32 cy = 0;
    for (size_t i = 0; i < a->block_num; ++i) {
        u32 tmp = c->block[i];
        c->block[i] = a->block[i] + tmp + cy;
        cy = (u32)(((u64) a->block[i] + (u64) tmp) >> 32);
    }
    for (size_t i = a->block_num; i < c->block_num; ++i) {
        u32 tmp = c->block[i];
        c->block[i] = tmp + cy;
        cy = (u32)(((u64) tmp + cy) >> 32);
    }
    if (cy) {
        big_num_resize(c, c->block_num + 1);
        c->block[c->block_num - 1] = cy;
    }
}

void big_num_square(big_num_t *c, big_num_t *a)
{
    if (!a)
        return;
    big_num_t *b = big_num_dup(a);
    big_num_mul(c, a, b);
    big_num_free(b);
}
big_num_t *big_num_dup(big_num_t *a)
{
    big_num_t *b = kvmalloc(sizeof(big_num_t), GFP_KERNEL);
    if (!b)
        return NULL;
    b->block = kvmalloc(sizeof(u32) * a->block_num, GFP_KERNEL);
    if (!b->block) {
        kvfree(b);
        return NULL;
    }
    b->block_num = a->block_num;
    memcpy(b->block, a->block, sizeof(u32) * a->block_num);
    return b;
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
    while (a->block_num < num) {
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