#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#define FIB_DEV "/dev/fibonacci"
#define SAMPLE_TIME 21
#define OFFSET 100
int cmpfunc(const void *a, const void *b)
{
    return (*(int *) a - *(int *) b);
}
int main()
{
    char big_num_buf[100];
    char write_buf[] = "testing writing";
    // int OFFSET = 100; /* TODO: try test something bigger than the limit */
    int **sample_kernel = malloc((OFFSET + 1) * sizeof(int *));
    int **sample_user = malloc((OFFSET + 1) * sizeof(int *));
    FILE *fpm = fopen("scripts/stat_med.txt", "w");
    struct timespec t1, t2;
    int fd = open(FIB_DEV, O_RDWR);
    if (fd < 0) {
        perror("Failed to open character device");
        exit(1);
    }
    for (int i = 0; i <= OFFSET; i++) {
        sample_user[i] = malloc(SAMPLE_TIME * sizeof(int));
        sample_kernel[i] = malloc(SAMPLE_TIME * sizeof(int));
    }
    for (int j = 0; j < SAMPLE_TIME; j++) {
        for (int i = 0; i <= OFFSET; i++) {
            int used = 0;
            lseek(fd, i, SEEK_SET);
            long long sz, sz2;
            clock_gettime(CLOCK_MONOTONIC, &t1);
            sz = read(fd, big_num_buf, 2);
            clock_gettime(CLOCK_MONOTONIC, &t2);
            sz2 = write(fd, write_buf, 2);
            sample_kernel[i][j] = (int) sz2;
            sample_user[i][j] = (int) (t2.tv_nsec - t1.tv_nsec);
        }
    }
    for (int i = 0; i <= OFFSET; i++) {
        lseek(fd, i, SEEK_SET);
        long long sz3 = read(fd, big_num_buf, 2);
        printf("f_big_num(%d): %s\n", i, big_num_buf);
        if (sz3)
            printf("f_big_num(%d) is truncated\n", i);
        qsort(sample_kernel[i], SAMPLE_TIME, sizeof(int), cmpfunc);
        qsort(sample_user[i], SAMPLE_TIME, sizeof(int), cmpfunc);
        fprintf(fpm, "%d %d %d\n", i, sample_user[i][SAMPLE_TIME / 2],
                sample_kernel[i][SAMPLE_TIME / 2]);
    }
    close(fd);
    fclose(fpm);
    for (int i = 0; i <= OFFSET; i++) {
        free(sample_user[i]);
        free(sample_kernel[i]);
    }
    free(sample_kernel);
    free(sample_user);
    return 0;
}
