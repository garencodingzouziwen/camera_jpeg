/*****************************************************
 * 文件名：camera_picture.c
 * 文件描述：获取摄像头支持的格式，并拍照保存
 * 编写人：ZiWenZou
 * 编写日期：2019-09-03
 * 修改日期：2019-09-03
*****************************************************/
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <time.h>
#include <sys/time.h>
// 操作摄像头设备
#include <linux/videodev2.h>

#define CAMERA_DEVICE "/dev/video0"
#define MMAP_BUFFER_NUM  1

#define JPG "./out/%s.jpg"

typedef struct
{
    void *start;
    int length;
} BUFTYPE;

BUFTYPE *usr_buf;

/*
**初始化摄像头，open设备，设置非阻塞
*/
int Camera_Init(char *directory)
{
    struct v4l2_input inp;
    int fd = open(directory, O_RDWR | O_NONBLOCK);
    if (fd < 0)
    {
        perror("open directory error!");
        return -1;
    }
    inp.index = 0;
    if (-1 == ioctl(fd, VIDIOC_S_INPUT, &inp))
    {
        perror("ioctl VIDIOC_S_INPUT error!\n");
        return -2;
    }
    return fd;
}

/*
**打印摄像头支持的所有格式以及对应的分辨率
*/
int Camera_PintfInfo(int fd)
{
    // 查询打开的设备是否属于摄像头：设备video不一定是摄像头
    struct v4l2_capability cap;
    if (-1 == ioctl(fd, VIDIOC_QUERYCAP, &cap))
    {
        perror("ioctl VIDIOC_QUERYCAP");
        return -1;
    }
    if (cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)
    {
        printf("Driver Name: %s\n", cap.driver); //摄像头驱动名字
    }
    else
    {
        printf("open file is not video\n");
        close(fd);
        return -2;
    }
    // 查询摄像头可捕捉的图片类型，VIDIOC_ENUM_FMT: 枚举摄像头帧格式
    int i,j;
    struct v4l2_fmtdesc fmt;
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE; // 指定需要枚举的类型
    for(i=0;;i++)
    {
        fmt.index = i;
        if ( -1 == ioctl(fd, VIDIOC_ENUM_FMT, &fmt))
        {
            break;
        }
        /* 打印摄像头图片格式 */
        printf("Picture Format: %s\n", fmt.description);
        /* 查询该图像格式所支持的分辨率 */
        struct v4l2_frmsizeenum frmsize;
        frmsize.pixel_format = fmt.pixelformat;
        for (j = 0;; j++)
        {
            frmsize.index = j;
            if (-1 == ioctl(fd, VIDIOC_ENUM_FRAMESIZES, &frmsize)) // 获取所有图片分辨率完成
            {
                break;
            }
            /* 打印图片分辨率 */
            printf("width: %d height: %d\n", frmsize.discrete.width, frmsize.discrete.height);
        }
    }
    return 0;
}

/*
**set video capture ways(mmap)
*/
static int Camera_Config_mmap(int fd)
{
    unsigned int n_buffer = 0;
    /*to request frame cache, contain requested counts*/
    struct v4l2_requestbuffers reqbufs;
    memset(&reqbufs, 0, sizeof(reqbufs));
    reqbufs.count = MMAP_BUFFER_NUM; /*the number of buffer*/ //缓冲区个数
    reqbufs.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    reqbufs.memory = V4L2_MEMORY_MMAP;
    if (-1 == ioctl(fd, VIDIOC_REQBUFS, &reqbufs)) //申请缓冲区
    {
        perror("ioctl VIDIOC_REQBUFS error!\n");
        return -1;
    }
    n_buffer = reqbufs.count;
    usr_buf = calloc(reqbufs.count, sizeof(BUFTYPE)); //申请地址user_buf(void *start;int length;共8byte)
    if (usr_buf == NULL)
    {
        perror("usr_buf == NULL error!\n");
        return -2;
    }
    /*map kernel cache to user process*/
    for (n_buffer = 0; n_buffer < reqbufs.count; ++n_buffer)
    {
        //stand for a frame
        struct v4l2_buffer buf; //定义一帧数据buf
        memset(&buf, 0, sizeof(buf));
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = n_buffer;
        /*check the information of the kernel cache requested*/
        if (-1 == ioctl(fd, VIDIOC_QUERYBUF, &buf)) //获取数据帧地址和长度
        {
            perror("ioctl VIDIOC_QUERYBUF error!\n");
            return -3;
        }
        usr_buf[n_buffer].length = buf.length;                                                                          //数据帧的长度
        printf("usr_buf[n_buffer].length :%d\n", usr_buf[n_buffer].length);                                             //打印测试
        usr_buf[n_buffer].start = (char *)mmap(NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, buf.m.offset); //进程与设备数据帧地址关联起来
        //之后就可以可以用进程的数据地址访问设备文件的数据
        if (MAP_FAILED == usr_buf[n_buffer].start)
        {
            perror("mmap MAP_FAILED error!\n");
            return -4;
        }
    }
    return 0;
}

/*
**Cameras参数配置
*/
int Camera_Config(int fd)
{
    struct v4l2_format tv_fmt;  /* frame format */
    /*set the form of camera capture data*/
    tv_fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE; /*v4l2_buf_typea,camera must use V4L2_BUF_TYPE_VIDEO_CAPTURE*/
    tv_fmt.fmt.pix.width = 1280;
    tv_fmt.fmt.pix.height = 720;
    tv_fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG; /*V4L2_PIX_FMT_YYUV*/ //V4L2_PIX_FMT_YUYV //V4L2_PIX_FMT_MJPEG
    tv_fmt.fmt.pix.field = V4L2_FIELD_NONE;                                /*V4L2_FIELD_NONE*/
    if (ioctl(fd, VIDIOC_S_FMT, &tv_fmt) < 0)                              //设置设备工作格式
    {
        perror("ioctl VIDIOC_S_FMT error!\n");
        return -1;
    }
    printf("user config camera pixelformat = %s,width = %d,height = %d\n", "MJPEG", 1280, 720);
    if(Camera_Config_mmap(fd) < 0) //内存映射
    {
        perror("Camera_Config_mmap() error!\n");
        return -2;
    }
    return 0;
}

/*
**Camera_StartCapture()开启捕获
*/
int Camera_StartCapture(int fd)
{
    unsigned int i;
    enum v4l2_buf_type type;
    /*place the kernel cache to a queue*/
    for (i = 0; i < MMAP_BUFFER_NUM; i++)
    {
        struct v4l2_buffer buf;
        memset(&buf, 0, sizeof(buf));
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;
        if (-1 == ioctl(fd, VIDIOC_QBUF, &buf))
        {
            perror("ioctl VIDIOC_QBUF error!\n");
            return -1;
        }
    }
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (-1 == ioctl(fd, VIDIOC_STREAMON, &type)) //启动采集数据
    {
        perror("ioctl VIDIOC_STREAMON error!\n");
        return -2;
    }
    return 0;
}

static int get_time_string(char *str)
{
    struct timeval tv;
    struct timezone tz;   
    struct tm *t;
    gettimeofday(&tv, &tz);
    t = localtime(&tv.tv_sec);
    sprintf(str,"%04d%02d%02d%02d%02d%02d", 1900 + t->tm_year, 1 + t->tm_mon, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec);
    return 0;
}

static int process_image(void *addr, int length)
{
    FILE *fp;
    char image_name[50]={0};
    char time_str[14]={0};
    get_time_string(time_str);
    sprintf(image_name, JPG, time_str);
    if ((fp = fopen(image_name, "w")) == NULL)
    {
        perror("Fail to fopen");
        return -1;
    }
    fwrite(addr, length, 1, fp);
    usleep(500);
    fclose(fp);
    return 0;
}

static int Camera_ReadCapture_picture(int fd)
{
    struct v4l2_buffer buf; //定义一帧数据buf
    memset(&buf, 0, sizeof(buf));
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    if (-1 == ioctl(fd, VIDIOC_DQBUF, &buf)) //从缓冲区取出一帧数据
    {
        perror("Fail to ioctl 'VIDIOC_DQBUF'");
        return -1;
    }
    assert(buf.index < MMAP_BUFFER_NUM);
    //read process space's data to a file
    process_image(usr_buf[buf.index].start, usr_buf[buf.index].length);
    if (-1 == ioctl(fd, VIDIOC_QBUF, &buf))
    {
        perror("Fail to ioctl 'VIDIOC_QBUF'");
        return -2;
    }
    return 0;
}

/*
**Camera_StartCapture()开启捕获
*/
int Camera_ReadCapture(int fd)
{
    fd_set fds;
    struct timeval tv;
    int r;
    FD_ZERO(&fds);
    FD_SET(fd, &fds);
    /*Timeout*/
    tv.tv_sec = 2;
    tv.tv_usec = 0;
    r = select(fd + 1, &fds, NULL, NULL, &tv); //等到一帧数据到来
    if (-1 == r)
    {
        perror("select error return -1!\n");
        return -1;
    }
    if (0 == r) //2秒都没有一帧数据-->超时处理
    {
        perror("select Timeout!!\n");
        return -2;
    }
    if (Camera_ReadCapture_picture(fd) < 0) //读取一帧数据并生成一张图片
    {
        perror("Camera_ReadCapture_frame error!\n");
        return -3;
    }
    return 0;
}

int Camera_StopCapture(int fd)
{
    unsigned int i;
    enum v4l2_buf_type type;
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (-1 == ioctl(fd, VIDIOC_STREAMOFF, &type))
    {
        perror("Fail to ioctl 'VIDIOC_STREAMOFF'");
        return -1;
    }
    for (i = 0; i < MMAP_BUFFER_NUM; i++)
    {
        if (-1 == munmap(usr_buf[i].start, usr_buf[i].length))
        {
            perror("munmap error!");
            return -2;
        }
    }
    free(usr_buf);
    return 0;
}

/*
**主函数main()
*/
int main(int argc, char **argv)
{
    int fd;
    //初始化摄像头
    if ((fd = Camera_Init(CAMERA_DEVICE)) < 0)
    {
        perror("Camera_Init() error!\n");
        return -1;
    }
    //打印支持的格式以及对应的分辨率
    if (-1 == Camera_PintfInfo(fd))
    {
        perror("Camera_PintfInfo() error!\n");
        return -2;
    }
    //设置摄像头参数
    if (Camera_Config(fd) < 0)
    {
        perror("Camera_Config() error!\n");
        return -3;
    }
    //启动摄像头数据流
    if (Camera_StartCapture(fd) < 0)
    {
        perror("Camera_StartCapture() error!\n");
        return -4;
    }
    //读取数据
    if (Camera_ReadCapture(fd) < 0)
    {
        perror("Camera_ReadCapture() error!\n");
        return -5;
    }
    //关闭摄像头数据包
    if (Camera_StopCapture(fd) < 0)
    {
        perror("Camera_StopCapture() error!\n");
        return -6;
    }
    //关闭fd
    close(fd);
    return 0;
}
