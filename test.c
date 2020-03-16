#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

struct BigNum {
    uint64_t lower;
    uint64_t upper;
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

void displayBit(struct BigNum x)
{
    unsigned long long display = (1ULL << 63);
    if (x.upper != 0) {
        uint8_t n_upper = __builtin_clzll(x.upper);
        // uint8_t n_lower = __builtin_clzll(x.lower);
        x.upper <<= n_upper;
        n_upper = 64 - n_upper;
        for (uint8_t i = 0; i < n_upper; ++i) {
            putchar((x.upper & display) ? '1' : '0');
            x.upper <<= 1;
        }
        // x.lower <<= n_lower;
        // n_lower = 64 - n_lower;
        for (uint8_t i = 0; i < 64; ++i) {
            putchar((x.lower & display) ? '1' : '0');
            x.lower <<= 1;
        }
    } else {
        uint8_t n_lower = __builtin_clzll(x.lower);
        x.lower <<= n_lower;
        n_lower = 64 - n_lower;
        for (uint8_t i = 0; i < n_lower; ++i) {
            putchar((x.lower & display) ? '1' : '0');
            x.lower <<= 1;
        }
    }
    putchar('\n');

    // for (int i = 1; i <= 64; i++) {
    //     putchar((x.upper & display) ? '1' : '0');
    //     display >>= 1;
    //     if (i % 8 == 0)
    //         putchar(' ');
    // }
    // display = (1ULL << 63);
    // // putchar('|');
    // for (int i = 1; i <= 64; i++) {
    //     putchar((x.lower & display) ? '1' : '0');
    //     display >>= 1;
    //     if (i % 8 == 0)
    //         putchar(' ');
    // }
    // putchar('\n');
    return;
}


void BigNumber_Mul(struct BigNum x, struct BigNum y, struct BigNum *output)
{
    uint8_t bit_shift = 0;
    output->upper = 0ULL;
    output->lower = 0ULL;
    struct BigNum tmp = x;

    while (y.lower != 0ULL || y.upper != 0ULL) {
        tmp = x;
        if (y.lower & 1ULL) {
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
        output->lower = ~(0ULL) - y->lower;
        return;
    }
    output->lower = x->lower - y->lower;
    return;
}

static struct BigNum fib_fast_doubling_clz(long long k)
{
    unsigned int n = 0;
    struct BigNum f_n = {.lower = 0ULL, .upper = 0ULL};
    struct BigNum f_n_1 = {.lower = 1ULL, .upper = 0ULL};

    n = __builtin_clzll(k);
    k <<= n;
    n = 64 - n;

    for (unsigned int i = 0; i < n; ++i) {
        struct BigNum f_2n_1 = {.lower = 0ULL, .upper = 0ULL};
        struct BigNum f_2n = {.lower = 0ULL, .upper = 0ULL};
        struct BigNum tmp1 = {.lower = 0ULL, .upper = 0ULL};
        struct BigNum tmp2 = {.lower = 0ULL, .upper = 0ULL};
        struct BigNum tmp3 = {.lower = 0ULL, .upper = 0ULL};

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

int main(int argc, char *argv[])
{
    struct BigNum Z = fib_fast_doubling_clz(atoi(argv[1]));
    displayBit(Z);
}