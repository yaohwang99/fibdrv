#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kdev_t.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include "big_num.h"

MODULE_LICENSE("Dual MIT/GPL");
MODULE_AUTHOR("National Cheng Kung University, Taiwan");
MODULE_DESCRIPTION("Fibonacci engine driver");
MODULE_VERSION("0.1");

#define DEV_FIBONACCI_NAME "fibonacci"

/* MAX_LENGTH is set to 92 because
 * ssize_t can't fit the number > 92
 */
#define MAX_LENGTH 1000

static dev_t fib_dev = 0;
static struct cdev *fib_cdev;
static struct class *fib_class;
static DEFINE_MUTEX(fib_mutex);

big_num_t *fib_sequence_big_num(long long k)
{
    big_num_t *a = big_num_create(1, 0);
    if (unlikely(!k))
        return a;
    big_num_t *b = big_num_create(1, 1);
    big_num_t *c = big_num_create(1, 1);

    for (int i = 2; i <= k; ++i) {
        big_num_add(c, a, b);
        big_num_t *t = a;
        a = b;
        b = c;
        c = t;
    }
    big_num_free(a);
    big_num_free(c);
    return b;
}

static long long fib_sequence(long long k)
{
    /* FIXME: C99 variable-length array (VLA) is not allowed in Linux kernel. */
    long long f[3];
    if (unlikely(k < 2))
        return (long long) k;
    f[0] = 0;
    f[1] = 1;
    for (int i = 2; i <= k; i++) {
        f[2] = f[1] + f[0];
        f[0] = f[1];
        f[1] = f[2];
    }

    return f[2];
}

big_num_t *fib_sequence_big_num_fdouble(long long k)
{
    size_t block_num = k / 46 + 1;
    big_num_t *fk = big_num_create(block_num, 0);
    if (unlikely(!k))
        return fk;
    big_num_t *fk1 = big_num_create(block_num, 1);
    big_num_t *f2k = big_num_create(block_num, 0);
    big_num_t *f2k1 = big_num_create(block_num, 0);

    big_num_t *t1 = big_num_create(block_num, 0);
    big_num_t *t2 = big_num_create(block_num, 0);

    long long m = 1 << (63 - __builtin_clz(k));
    while (m) {
        // f2k = fk * (2 * fk1 - fk);
        big_num_cpy(t1, fk1);
        big_num_add(t2, fk1, t1);
        big_num_sub(t1, t2, fk);
        big_num_mul(f2k, fk, t1);
        // f2k1 = fk * fk + fk1 * fk1;
        big_num_square(t1, fk);
        big_num_square(t2, fk1);
        big_num_add(f2k1, t1, t2);
        if (k & m) {
            big_num_cpy(fk, f2k1);
            big_num_add(fk1, f2k, f2k1);
        } else {
            big_num_cpy(fk, f2k);
            big_num_cpy(fk1, f2k1);
        }
        m >>= 1;
    }
    big_num_free(fk1);
    big_num_free(f2k);
    big_num_free(f2k1);
    big_num_free(t1);
    big_num_free(t2);
    return fk;
}

static long long fib_sequence_fdouble(long long k)
{
    if (unlikely(k < 2))
        return k;
    long long fk = 0;
    long long m = 1 << (63 - __builtin_clz(k));

    while (m) {
        long long fk1 = 1, f2k = 0, f2k1;
        f2k = fk * (2 * fk1 - fk);
        f2k1 = fk * fk + fk1 * fk1;
        if (k & m) {
            fk = f2k1;
            fk1 = f2k + f2k1;
        } else {
            fk = f2k;
            fk1 = f2k1;
        }
        m >>= 1;
    }
    return fk;
}

static int fib_open(struct inode *inode, struct file *file)
{
    if (!mutex_trylock(&fib_mutex)) {
        printk(KERN_ALERT "fibdrv is in use");
        return -EBUSY;
    }
    return 0;
}

static int fib_release(struct inode *inode, struct file *file)
{
    mutex_unlock(&fib_mutex);
    return 0;
}

/* calculate the fibonacci number at given offset */
static ssize_t fib_read(struct file *file,
                        char *buf,
                        size_t size,
                        loff_t *offset)
{
    big_num_t *a = NULL;
    char *p;
    char *r;
    size_t len = 0;
    ssize_t sz;
    switch (size) {
    case 1:
        return (ssize_t) fib_sequence_fdouble(*offset);
    case 2:
        a = fib_sequence_big_num(*offset);
        p = big_num_to_string(a);
        r = p;
        for (; *r == '0' && *(r + 1); ++r)
            ;
        len = strlen(r) + 1;
        sz = copy_to_user(buf, r, len);
        kvfree(p);
        return sz;
    case 3:
        a = fib_sequence_big_num_fdouble(*offset);
        p = big_num_to_string(a);
        r = p;
        for (; *r == '0' && *(r + 1); ++r)
            ;
        len = strlen(r) + 1;
        sz = copy_to_user(buf, r, len);
        kvfree(p);
        return sz;
    default:
        return (ssize_t) fib_sequence(*offset);
    }
}

/* write operation is skipped */
static ssize_t fib_write(struct file *file,
                         const char *buf,
                         size_t size,
                         loff_t *offset)
{
    ktime_t kt;
    big_num_t *a = NULL;
    switch (size) {
    case 1:
        return 1;
    case 2:
        kt = ktime_get();
        a = fib_sequence_big_num(*offset);
        kt = ktime_sub(ktime_get(), kt);
        big_num_free(a);
        return (ssize_t) ktime_to_ns(kt);
    case 3:
        kt = ktime_get();
        a = fib_sequence_big_num_fdouble(*offset);
        kt = ktime_sub(ktime_get(), kt);
        big_num_free(a);
        return (ssize_t) ktime_to_ns(kt);
    default:
        kt = ktime_get();
        fib_sequence_fdouble(*offset);
        kt = ktime_sub(ktime_get(), kt);
        return (ssize_t) ktime_to_ns(kt);
    }
}

static loff_t fib_device_lseek(struct file *file, loff_t offset, int orig)
{
    loff_t new_pos = 0;
    switch (orig) {
    case 0: /* SEEK_SET: */
        new_pos = offset;
        break;
    case 1: /* SEEK_CUR: */
        new_pos = file->f_pos + offset;
        break;
    case 2: /* SEEK_END: */
        new_pos = MAX_LENGTH - offset;
        break;
    }

    if (new_pos > MAX_LENGTH)
        new_pos = MAX_LENGTH;  // max case
    if (new_pos < 0)
        new_pos = 0;        // min case
    file->f_pos = new_pos;  // This is what we'll use now
    return new_pos;
}

const struct file_operations fib_fops = {
    .owner = THIS_MODULE,
    .read = fib_read,
    .write = fib_write,
    .open = fib_open,
    .release = fib_release,
    .llseek = fib_device_lseek,
};

static int __init init_fib_dev(void)
{
    int rc = 0;

    mutex_init(&fib_mutex);
    // Let's register the device
    // This will dynamically allocate the major number
    rc = alloc_chrdev_region(&fib_dev, 0, 1, DEV_FIBONACCI_NAME);

    if (rc < 0) {
        printk(KERN_ALERT
               "Failed to register the fibonacci char device. rc = %i",
               rc);
        return rc;
    }

    fib_cdev = cdev_alloc();
    if (fib_cdev == NULL) {
        printk(KERN_ALERT "Failed to alloc cdev");
        rc = -1;
        goto failed_cdev;
    }
    fib_cdev->ops = &fib_fops;
    rc = cdev_add(fib_cdev, fib_dev, 1);

    if (rc < 0) {
        printk(KERN_ALERT "Failed to add cdev");
        rc = -2;
        goto failed_cdev;
    }

    fib_class = class_create(THIS_MODULE, DEV_FIBONACCI_NAME);

    if (!fib_class) {
        printk(KERN_ALERT "Failed to create device class");
        rc = -3;
        goto failed_class_create;
    }

    if (!device_create(fib_class, NULL, fib_dev, NULL, DEV_FIBONACCI_NAME)) {
        printk(KERN_ALERT "Failed to create device");
        rc = -4;
        goto failed_device_create;
    }
    return rc;
failed_device_create:
    class_destroy(fib_class);
failed_class_create:
    cdev_del(fib_cdev);
failed_cdev:
    unregister_chrdev_region(fib_dev, 1);
    return rc;
}

static void __exit exit_fib_dev(void)
{
    mutex_destroy(&fib_mutex);
    device_destroy(fib_class, fib_dev);
    class_destroy(fib_class);
    cdev_del(fib_cdev);
    unregister_chrdev_region(fib_dev, 1);
}

module_init(init_fib_dev);
module_exit(exit_fib_dev);
