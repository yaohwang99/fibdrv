#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#define FIB_DEV "/dev/fibonacci"
#define SAMPLE_TIME 10000
#define OFFSET 10000
#define METHOD 3

int main()
{
    char write_buf[] = "testing writing";
    int fd = open(FIB_DEV, O_RDWR);
    if (fd < 0) {
        perror("Failed to open character device");
        exit(1);
    }

    for (int j = 0; j < SAMPLE_TIME; j++) {
        for (int i = OFFSET; i <= OFFSET; i++) {
            int used = 0;
            lseek(fd, i, SEEK_SET);
            long long sz2;
            sz2 = write(fd, write_buf, METHOD);
        }
    }

    close(fd);
    return 0;
}
