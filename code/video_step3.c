#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/videodev2.h>
#include <string.h>

int main(void)
{
    // 1.打开设备
    int fd = open("/dev/video0", O_RDWR);
    if (fd < 0)
    {
        perror("打开设备失败");
        return -1;
    }

    // 2.获取摄像头支持的格式,ioctl(文件描述符，命令，与命令对应的结构体)
    struct v4l2_fmtdesc v4fmt;
    v4fmt.index = 0;
    v4fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    int ret = ioctl(fd, VIDIOC_ENUM_FMT, &v4fmt);
    if (ret < 0)
    {
        perror("获取摄像头支持格式失败");
    }
    printf("flags=%d\n", v4fmt.flags);
    printf("description=%s\n", v4fmt.description);
    unsigned char *p = (unsigned char *)&v4fmt.pixelformat;
    printf("pixelformat=%c%c%c%c\n", p[0], p[1], p[2], p[3]);
    printf("reserved=%d\n", v4fmt.reserved[0]);

    // 3.设置采集格式
    struct v4l2_format vfmt;
    vfmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE; // 摄像头采集
    vfmt.fmt.pix.width = 640;                // 设置宽
    vfmt.fmt.pix.height = 480;               // 设置高
    vfmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
    ret = ioctl(fd, VIDIOC_S_FMT, &vfmt);
    if (ret < 0)
    {
        perror("设置采集格式失败");
    }
    // 清空后重新获取
    memset(&vfmt, 0, sizeof(vfmt));
    vfmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    ret = ioctl(fd, VIDIOC_G_FMT, &vfmt);
    if (ret < 0)
    {
        perror("获取格式失败");
    }
    if (vfmt.fmt.pix.width == 640 && vfmt.fmt.pix.height == 480 && vfmt.fmt.pix.pixelformat == V4L2_PIX_FMT_YUYV)
    {
        printf("设置成功\n");
    }
    else
    {
        printf("设置失败\n");
    }

    // 9.关闭设备
    close(fd);
    return 0;
}