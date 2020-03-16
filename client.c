#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#define FIB_DEV "/dev/fibonacci"
#define BUF_LEN 10
#define FIB_INPUT 94

struct BigNum {
    unsigned long long lower;
    unsigned long long upper;
};

void displayBit(struct BigNum x)
{
    unsigned long long display = (1ULL << 63);
    if (x.upper != 0) {
        uint8_t n_upper = __builtin_clzll(x.upper);
        x.upper <<= n_upper;
        n_upper = 64 - n_upper;
        for (uint8_t i = 0; i < n_upper; ++i) {
            putchar((x.upper & display) ? '1' : '0');
            x.upper <<= 1;
        }
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
    return;
}

int main()
{
    char *buf = (char *) malloc(sizeof(struct BigNum));
    // char write_buf[] = "testing writing";
    int offset = FIB_INPUT; /* TODO: try test something bigger than the limit */
    // FILE *fp = fopen("./result.txt", "w");
    // char print_buf[BUF_LEN];


    int fd = open(FIB_DEV, O_RDWR);
    if (fd < 0) {
        perror("Failed to open character device");
        exit(1);
    }

    // for (int i = 0; i <= offset; i++) {
    //     // long long sz;
    //     lseek(fd, i, SEEK_SET);
    //     read(fd, buf, sizeof(struct BigNum));
    //     // displayBit((struct BigNum *)buf);
    //     // sz = read(fd, buf, 1);
    //     if (((struct BigNum *)buf)->upper) {
    //         printf("Reading from " FIB_DEV
    //            " at offset %d, returned the sequence "
    //            "%llu%llu.\n",
    //            i, ((struct BigNum *)buf)->upper, ((struct BigNum
    //            *)buf)->lower);
    //     } else {
    //         printf("Reading from " FIB_DEV
    //            " at offset %d, returned the sequence "
    //            "%llu.\n",
    //            i,((struct BigNum *)buf)->lower);
    //     }

    //     // sz = write(fd, write_buf, strlen(write_buf));
    //     // printf("Writing to " FIB_DEV ", returned the sequence %llu\n",
    //     sz);
    //     // int length = snprintf(print_buf, BUF_LEN, "%d %llu\n", i, sz);
    //     // if (length >= BUF_LEN) {
    //     //     close(fd);
    //     //     fclose(fp);
    //     //     return EXIT_FAILURE;
    //     // }

    //     // fputs(print_buf, fp);
    // }

    lseek(fd, offset, SEEK_SET);
    read(fd, buf, sizeof(struct BigNum));
    if (((struct BigNum *) buf)->upper) {
        printf("%llu%llu\n", ((struct BigNum *) buf)->upper,
               ((struct BigNum *) buf)->lower);
    } else {
        printf("%llu\n", ((struct BigNum *) buf)->lower);
    }

    // for (int i = offset; i >= 0; i--) {
    //     lseek(fd, i, SEEK_SET);
    //     read(fd, buf, sizeof(struct BigNum));
    //     if (((struct BigNum *)buf)->upper) {
    //         printf("Reading from " FIB_DEV
    //            " at offset %d, returned the sequence "
    //            "%llu%llu.\n",
    //            i, ((struct BigNum *)buf)->upper, ((struct BigNum
    //            *)buf)->lower);
    //     } else {
    //         printf("Reading from " FIB_DEV
    //            " at offset %d, returned the sequence "
    //            "%llu.\n",
    //            i,((struct BigNum *)buf)->lower);
    //     }
    // }

    close(fd);
    // fclose(fp);
    return 0;
}
