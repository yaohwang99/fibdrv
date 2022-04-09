#include "big_num.h"
void big_num_swap(big_num_t **a, big_num_t **b)
{
    big_num_t *tmp = *a;
    *a = *b;
    *b = tmp;
}
base_t base_add(base_t *c, base_t a, base_t b, base_t cy)
{
    cy = (a += cy) < cy;
    cy += (*c = a + b) < a;
    return cy;
}
// c = a + b
void big_num_add(big_num_t *c, big_num_t *a, big_num_t *b)
{
    if (!a || !b)
        return;
    if (a->block_num < b->block_num) {
        big_num_t *t = a;
        a = b;
        b = t;
    }
    big_num_resize(c, a->block_num);
    base_t cy = 0;
    for (size_t i = 0; i < b->block_num; ++i) {
        cy = base_add(&c->block[i], a->block[i], b->block[i], cy);
    }
    for (size_t i = b->block_num; i < a->block_num; ++i) {
        cy = base_add(&c->block[i], a->block[i], 0, cy);
    }
    if (cy) {
        big_num_resize(c, a->block_num + 1);
        c->block[c->block_num - 1] = cy;
    }
    big_num_trim(c);
}

// c = a - b, assume a > b
void big_num_sub(big_num_t *c, big_num_t *a, big_num_t *b)
{
    if (!a || !b)
        return;
    big_num_resize(b, a->block_num);
    big_num_resize(c, a->block_num);
    if (a->block_num == 1) {
        c->block[0] = a->block[0] - b->block[0];
        return;
    }
    base_t cy = 1;
    for (size_t i = 0; i < a->block_num; ++i) {
        cy = base_add(&c->block[i], a->block[i], ~b->block[i], cy);
    }
    big_num_trim(c);
}
void big_num_lshift(big_num_t *a, int k)
{
    base_t cy = 0;
    for (size_t i = 0; i < a->block_num; ++i) {
        base_t t = a->block[i];
        a->block[i] = (t << k) | cy;
        cy = t >> (BASE_BITS - k);
    }
    if (cy) {
        big_num_resize(a, a->block_num + 1);
        a->block[a->block_num - 1] = cy;
    }
}
base_t base_mul(base_t *c, base_t a, base_t b, base_t cy)
{
    base_t hi;
    __asm__("mulq %3" : "=a"(*c), "=d"(hi) : "%0"(a), "rm"(b));
    cy = ((*c += cy) < cy) + hi;
    return cy;
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
    // short cut
    if (a->block_num == 1 && b->block_num == 1) {
        base_t cy = 0;
        cy = base_mul(&c->block[0], a->block[0], b->block[0], cy);
        c->block[1] = cy;
        big_num_trim(c);
        return;
    }

    if (!c)
        return;
    for (size_t shift = 0; shift < b->block_num; ++shift) {
        base_t cy = 0;
        size_t i = 0;
        for (; i < a->block_num; ++i) {
            base_t t;
            cy = base_mul(&t, a->block[i], b->block[shift], cy);
            cy += base_add(&c->block[i + shift], c->block[i + shift], t, 0);
        }
        for (int j = 0; cy != 0; ++j) {
            cy = base_add(&c->block[i + j + shift], c->block[i + j + shift], 0,
                          cy);
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

    // short cut
    if (a->block_num == 1) {
        base_t cy = 0;
        cy = base_mul(&c->block[0], a->block[0], a->block[0], cy);
        c->block[1] = cy;
        big_num_trim(c);
        return;
    }

    for (size_t shift = 0; shift < a->block_num; ++shift) {
        base_t cy = 0;
        size_t i = shift + 1;
        for (; i < a->block_num; ++i) {
            base_t t;
            cy = base_mul(&t, a->block[i], a->block[shift], cy);
            cy += base_add(&c->block[i + shift], c->block[i + shift], t, 0);
        }
        for (int j = 0; cy != 0; ++j) {
            cy = base_add(&c->block[i + j + shift], c->block[i + j + shift], 0,
                          cy);
        }
    }
    big_num_lshift(c, 1);
    base_t cy = 0;
    for (size_t shift = 0; shift < a->block_num; ++shift) {
        base_t t;
        cy = base_mul(&t, a->block[shift], a->block[shift], cy);
        cy += base_add(&c->block[2 * shift], c->block[2 * shift], t, 0);
        for (int j = 1; cy != 0; ++j) {
            cy = base_add(&c->block[j + 2 * shift], c->block[j + 2 * shift], 0,
                          cy);
        }
    }
    big_num_trim(c);
}


big_num_t *big_num_create(size_t num, base_t init)
{
    big_num_t *a = kvmalloc(sizeof(big_num_t), GFP_KERNEL);
    if (!a)
        return NULL;
    a->block = kvmalloc(sizeof(base_t) * num, GFP_KERNEL);
    if (!a->block) {
        kvfree(a);
        return NULL;
    }
    a->true_block_num = num;
    a->block_num = 1;
    memset(a->block, 0, sizeof(base_t) * num);
    a->block[0] = init;
    return a;
}
char *big_num_to_string(big_num_t *a)
{
    size_t len = (a->block_num * sizeof(base_t) * 8) / 3 + 2;
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
        for (base_t m = 1ull << (BASE_BITS - 1); m; m >>= 1) {
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
    memset(a->block, 0, sizeof(base_t) * a->block_num);
}
void big_num_resize(big_num_t *a, int num)
{
    // decrease size
    if (a->true_block_num >= num) {
        a->block_num = num;
    } else {  // num > true block num >= block num
        a->block = kvrealloc(a->block, sizeof(base_t) * a->true_block_num,
                             sizeof(base_t) * num, GFP_KERNEL);

        memset(&a->block[a->true_block_num], 0,
               sizeof(base_t) * (num - a->true_block_num));
        a->true_block_num = a->block_num = num;
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