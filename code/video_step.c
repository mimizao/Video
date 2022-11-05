#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/videodev2.h>
#include <string.h>
#include <sys/mman.h>

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
    struct v4l2_fmtdesc v4fmt =
        {
            .index = 0,
            .type = V4L2_BUF_TYPE_VIDEO_CAPTURE,
        };
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
    struct v4l2_format vfmt =
        {
            .type = V4L2_BUF_TYPE_VIDEO_CAPTURE,      // 摄像头采集
            .fmt.pix.width = 160,                     // 设置宽
            .fmt.pix.height = 120,                    // 设置高
            .fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV, // 设置视频采集格式
        };
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
    if (vfmt.fmt.pix.width == 160 && vfmt.fmt.pix.height == 120 && vfmt.fmt.pix.pixelformat == V4L2_PIX_FMT_YUYV)
    {
        printf("设置成功\n");
    }
    else
    {
        printf("设置失败\n");
    }

    // 4.申请内核空间
    struct v4l2_requestbuffers reqbuffers =
        {
            .type = V4L2_BUF_TYPE_VIDEO_CAPTURE,
            .count = 4,                 // 申请4个缓存区
            .memory = V4L2_MEMORY_MMAP, // 映射方式
        };
    ret = ioctl(fd, VIDIOC_REQBUFS, &reqbuffers);
    if (ret < 0)
    {
        perror("申请队列空间失败");
    }

    // 5.映射队列空间到用户空间
    unsigned char *mptr[4]; // 保存映射后用户空间的首地址
    unsigned int size[4];
    struct v4l2_buffer mapbuffer;

    for (unsigned int n_buffers = 0; n_buffers < reqbuffers.count; n_buffers++)
    {
        memset(&mapbuffer, 0, sizeof(mapbuffer));
        mapbuffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        mapbuffer.memory = V4L2_MEMORY_MMAP;
        mapbuffer.index = n_buffers;                  // 初始化index
        ret = ioctl(fd, VIDIOC_QUERYBUF, &mapbuffer); // 从内核空间中查询一个空间做映射
        if (ret < 0)
        {
            perror("查询内核空间队列失败");
        }
        mptr[n_buffers] = (unsigned char *)mmap(NULL, mapbuffer.length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, mapbuffer.m.offset);
        size[n_buffers] = mapbuffer.length;
        // 通知内核使用完毕，放回去
        ret = ioctl(fd, VIDIOC_QBUF, &mapbuffer);
        if (ret < 0)
        {
            perror("放回失败");
        }
    }

    // 6.开始采集
    int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    ret = ioctl(fd, VIDIOC_STREAMON, &type);
    if (ret < 0)
    {
        perror("开启失败");
    }

    // 7.开始采集
    // 从队列中提取一帧数据
    struct v4l2_buffer readbuffer =
        {
            .type = V4L2_BUF_TYPE_VIDEO_CAPTURE,
        };
    ret = ioctl(fd, VIDIOC_DQBUF, &readbuffer);
    if (ret < 0)
    {
        perror("提取数据失败");
    }
    FILE *file = fopen("my.yuyv", "w+");
    fwrite(mptr[readbuffer.index], readbuffer.length, 1, file);
    fclose(file);
    // 通知内核已经使用完毕
    ret = ioctl(fd, VIDIOC_QBUF, &readbuffer);
    if (ret < 0)
    {
        perror("放回队列失败");
    }

    // 8.停止采集
    ret = ioctl(fd, VIDIOC_STREAMOFF, &type);

    // 9.释放映射
    for (int i = 0; i < 4; i++)
    {
        munmap(mptr[i], size[i]);
    }

    // 10.关闭设备
    close(fd);
    return 0;
}