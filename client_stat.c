#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#define FIB_DEV "/dev/fibonacci"
#define SAMPLE_TIME 10
#define TIME_BUF_SIZE 2048
int main()
{
    char buf[1];

    char write_buf[] = "testing writing";
    int offset = 100; /* TODO: try test something bigger than the limit */
    FILE *fp = fopen("scripts/stat.txt", "w");
    struct timespec t1, t2;
    int fd = open(FIB_DEV, O_RDWR);
    if (fd < 0) {
        perror("Failed to open character device");
        exit(1);
    }
    for (int i = 0; i <= offset; i++) {
        char time_buf[TIME_BUF_SIZE];
        int used = 0;
        for (int j = 0; j < SAMPLE_TIME; j++) {
            long long sz, sz2;
            lseek(fd, i, SEEK_SET);
            clock_gettime(CLOCK_MONOTONIC, &t1);
            sz = read(fd, buf, 1);
            clock_gettime(CLOCK_MONOTONIC, &t2);
            sz2 = write(fd, write_buf, 0);
            snprintf(&time_buf[used], TIME_BUF_SIZE - used, "%ld %lld ",
                     (long int) (t2.tv_nsec - t1.tv_nsec), sz2);
            used = strnlen(time_buf, TIME_BUF_SIZE);
        }
        fprintf(fp, "%d %s\n", i, time_buf);
    }
    close(fd);
    fclose(fp);
    return 0;
}
