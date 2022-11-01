#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
int main(void)
{
    // 1.打开设备
    int fd = open("/dev/video0", O_RDWR);
    if (fd < 0)
    {
        perror("打开设备失败");
        return -1;
    }

    // 9.关闭设备
    close(fd);
    return 0;
}