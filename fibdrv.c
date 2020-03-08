#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kdev_t.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mutex.h>


MODULE_LICENSE("Dual MIT/GPL");
MODULE_AUTHOR("National Cheng Kung University, Taiwan");
MODULE_DESCRIPTION("Fibonacci engine driver");
MODULE_VERSION("0.1");

#define DEV_FIBONACCI_NAME "fibonacci"

/* MAX_LENGTH is set to 92 because
 * ssize_t can't fit the number > 92
 */
#define MAX_LENGTH 92
static dev_t fib_dev = 0;
static struct cdev *fib_cdev;
static struct class *fib_class;
static DEFINE_MUTEX(fib_mutex);
static ktime_t kt1, kt2;
struct BigNum {
    u64 lower;
    u64 upper;
};
void BigNumber_Add(const struct BigNum *x,
                   const struct BigNum *y,
                   struct BigNum *output)
{
    output->upper = x->upper + y->upper;

    if (y->lower > ~x->lower)
        output->upper++;

    output->lower = x->lower + y->lower;
    return;
}

void bit_shift_left(struct BigNum *x, uint8_t num)
{
    for (uint8_t i = 0; i < num; ++i) {
        x->upper <<= 1;
        if (x->lower & 0x8000000000000000) {
            x->lower <<= 1;
            x->upper++;
        } else {
            x->lower <<= 1;
        }
    }
    return;
}

void bit_shift_right(struct BigNum *x, uint8_t num)
{
    for (uint8_t i = 0; i < num; ++i) {
        x->lower >>= 1;
        if (x->upper & 0x1) {
            x->upper >>= 1;
            x->lower += 0x8000000000000000;
        } else {
            x->upper >>= 1;
        }
    }
    return;
}

void BigNumber_Mul(struct BigNum x, struct BigNum y, struct BigNum *output)
{
    uint8_t bit_shift = 0;
    output->upper = 0;
    output->lower = 0;
    struct BigNum tmp = x;

    while (y.lower != 0 || y.upper != 0) {
        tmp = x;
        if (y.lower & 1) {
            bit_shift_left(&tmp, bit_shift);
            BigNumber_Add(output, &tmp, output);
        }
        bit_shift++;
        bit_shift_right(&y, 1);
    }
    return;
}

void BigNumber_Sub(const struct BigNum *x,
                   const struct BigNum *y,
                   struct BigNum *output)
{
    output->upper = x->upper - y->upper;

    if (y->lower > x->lower) {
        output->upper--;
        output->lower = ~(x->lower << 63) - y->lower;
        return;
    }
    output->lower = x->lower - y->lower;
    return;
}

static struct BigNum fib_fast_doubling_clz(long long k)
{
    unsigned int n = 0;
    struct BigNum f_n = {.lower = 0, .upper = 0};
    struct BigNum f_n_1 = {.lower = 1, .upper = 0};

    n = __builtin_clzll(
        k);  // __builtin_clz  only for unsigned int (32 bits) __builtin_clzll,
             // ll means unsigned long long, so it is 64 bits
    k <<= n;
    n = 64 - n;

    for (unsigned int i = 0; i < n; ++i) {
        struct BigNum f_2n_1 = {.lower = 0, .upper = 0};
        struct BigNum f_2n = {.lower = 0, .upper = 0};
        struct BigNum tmp1 = {.lower = 0, .upper = 0};
        struct BigNum tmp2 = {.lower = 0, .upper = 0};
        struct BigNum tmp3 = {.lower = 0, .upper = 0};

        BigNumber_Add(&f_n_1, &f_n_1, &tmp1);
        BigNumber_Sub(&tmp1, &f_n, &tmp2);
        BigNumber_Mul(f_n, tmp2, &f_2n);

        BigNumber_Mul(f_n, f_n, &f_n);
        BigNumber_Mul(f_n_1, f_n_1, &tmp3);
        BigNumber_Add(&f_n, &tmp3, &f_2n_1);

        if (k & 0x8000000000000000) {
            f_n = f_2n_1;
            BigNumber_Add(&f_2n, &f_2n_1, &f_n_1);
        } else {
            f_n = f_2n;
            f_n_1 = f_2n_1;
        }
        k <<= 1;
    }
    return f_n;
}

static long long fib_fast_doubling(long long k)
{
    unsigned int n = 0;
    if (k == 0) {
        return 0;
    } else if (k <= 2) {
        return 1;
    }

    if (k % 2) {
        n = (k - 1) / 2;
        return fib_fast_doubling(n) * fib_fast_doubling(n) +
               fib_fast_doubling(n + 1) * fib_fast_doubling(n + 1);
    } else {
        n = k / 2;
        return fib_fast_doubling(n) *
               (2 * fib_fast_doubling(n + 1) - fib_fast_doubling(n));
    }
}

static long long fib_time_proxy(long long k)
{
    kt1 = ktime_get();
    fib_fast_doubling_clz(k);
    kt1 = ktime_sub(ktime_get(), kt1);

    kt2 = ktime_get();
    long long result = fib_fast_doubling(k);
    kt2 = ktime_sub(ktime_get(), kt2);

    printk(KERN_INFO "%lld %lld", ktime_to_ns(kt1), ktime_to_ns(kt2));
    return result;
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
    // return (ssize_t) fib_sequence(*offset);
    return (ssize_t) fib_time_proxy(*offset);
}

/* write operation is skipped */
static ssize_t fib_write(struct file *file,
                         const char *buf,
                         size_t size,
                         loff_t *offset)
{
    return 1;
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
    cdev_init(fib_cdev, &fib_fops);
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
