#include "v4l2.h"
#include <sys/mman.h>
#include <jpeglib.h>
#include <stdlib.h>
#include <poll.h>

/* 判断设备是否支持指定的图像格式 */
bool v4l2_testPixFormat(int fd, uint32_t fmt)
{
    struct v4l2_fmtdesc fmtdesc;
    fmtdesc.type=V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmtdesc.index=0;

    V4L2_LOG("Support formats:\n");
    while(ioctl(fd, VIDIOC_ENUM_FMT, &fmtdesc) != -1)
    {
        if(fmtdesc.pixelformat == fmt)
        {
            V4L2_LOG("\t->%d.%s\n",fmtdesc.index+1,fmtdesc.description);
            return true;
        }
        V4L2_LOG("\t  %d.%s\n",fmtdesc.index+1,fmtdesc.description);

        fmtdesc.index++;
    }
    
    return false;
}

/* 设置像素格式 */
bool v4l2_setPixFormat(int fd, uint32_t fmt)
{
    struct v4l2_format videoFormat;
    videoFormat.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    videoFormat.fmt.pix.pixelformat = fmt;

    if(ioctl(fd, VIDIOC_S_FMT, &videoFormat) < 0)
    {
        V4L2_LOG("ioctl VIDIOC_S_FMT failed: %s\n", strerror(errno));
        return false;
    }

    return true;
}

/* 获取像素格式 */
bool v4l2_getPixFormat(int fd, uint32_t* fmt)
{
    struct v4l2_format videoFormat;
    videoFormat.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if(ioctl(fd, VIDIOC_G_FMT, &videoFormat) < 0)
    {
        V4L2_LOG("ioctl VIDIOC_S_FMT failed: %s\n", strerror(errno));
        return false;
    }

    *fmt = videoFormat.fmt.pix.pixelformat;
    return true;
}

/* 获取分辨率 */
bool v4l2_getSize(int fd, uint32_t* width, uint32_t* height)
{
    struct v4l2_format videoFormat;
    videoFormat.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if(ioctl(fd, VIDIOC_G_FMT, &videoFormat) < 0)
    {
        V4L2_LOG("ioctl VIDIOC_S_FMT failed: %s\n", strerror(errno));
        return false;
    }

    *width = videoFormat.fmt.pix.width;
    *height = videoFormat.fmt.pix.height;
    return true;
}

/* 设置分辨率 */
bool v4l2_setSize(int fd, uint32_t width, uint32_t height)
{
    struct v4l2_format videoFormat;
    videoFormat.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    videoFormat.fmt.pix.width = width;
    videoFormat.fmt.pix.height = height;

    if(ioctl(fd, VIDIOC_S_FMT, &videoFormat) < 0)
    {
        V4L2_LOG("ioctl VIDIOC_S_FMT failed: %s\n", strerror(errno));
        return false;
    }

    return true;
}

/* 设置采集范围 */
bool v4l2_setArea(int fd, int x, int y, int width, int height)
{
    struct v4l2_crop crop;
    crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    crop.c.left = x;
    crop.c.top = y;
    crop.c.width = width;
    crop.c.height = height;

    if(ioctl(fd, VIDIOC_S_CROP, &crop) < 0)
    {
        V4L2_LOG("ioctl VIDIOC_S_CROP failed: %s\n", strerror(errno));
        return false;
    }

    return true;
}

/* 获取采集范围 */
bool v4l2_getArea(int fd, int* x, int* y, int* width, int* height)
{
    struct v4l2_crop crop;
    crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if(ioctl(fd, VIDIOC_S_CROP, &crop) < 0)
    {
        V4L2_LOG("ioctl VIDIOC_S_CROP failed: %s\n", strerror(errno));
        return false;
    }

    *x = crop.c.left;
    *y = crop.c.top;
    *width = crop.c.width;
    *height = crop.c.height;
    return true;
}

/* 获取帧率 */
bool v4l2_getFPS(int fd, uint32_t* fps)
{
    struct v4l2_streamparm streamParm;
    streamParm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if(ioctl(fd, VIDIOC_G_PARM, &streamParm) < 0)
    {
        fprintf(stderr, "ioctl VIDIOC_G_PARM failed: %s\n", strerror(errno));
        return false;
    }

    *fps = streamParm.parm.capture.timeperframe.denominator / streamParm.parm.capture.timeperframe.numerator;
    return true;
}

/* 设置帧率 */
bool v4l2_setFPS(int fd, uint32_t fps)
{
    struct v4l2_streamparm streamParm;
    streamParm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    streamParm.parm.capture.timeperframe.denominator = fps;
    streamParm.parm.capture.timeperframe.numerator = 1;
    if(ioctl(fd, VIDIOC_S_PARM, &streamParm) < 0)
    {
        fprintf(stderr, "ioctl VIDIOC_S_PARM failed: %s\n", strerror(errno));
        return false;
    }

    return true;
}

/* 申请指定数量的帧缓冲队列 */
bool v4l2_requestBuffer(int fd, uint32_t count)
{
    struct v4l2_requestbuffers request;
    request.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    request.memory = V4L2_MEMORY_MMAP;
    request.count = count;

    if(!ioctl(fd, VIDIOC_REQBUFS, &request) < 0)
    {
        V4L2_LOG("ioctl VIDIOC_REQBUFS failed: %s\n", strerror(errno));
        return false;
    }

    return true;
}

/* 获取帧缓冲的位置 */
bool v4l2_getBuffer(int fd, uint32_t index, struct v4l2_buffer* buffer)
{
    memset(buffer, 0, sizeof(*buffer));
    buffer->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buffer->memory = V4L2_MEMORY_MMAP;
    buffer->index = index;

    if(ioctl(fd, VIDIOC_QUERYBUF, buffer) < 0)
    {
        V4L2_LOG("ioctl VIDIOC_QUERYBUF failed: %s\n", strerror(errno));
        return false;
    }

    return true;
}

/* 映射缓冲数据指针 */
bool v4l2_mapBuffer(int fd, struct v4l2_buffer* buffer, void** ptr)
{
    *ptr = mmap(NULL, buffer->length, PROT_READ|PROT_WRITE, MAP_SHARED, fd, buffer->m.offset);
    if(*ptr == NULL)
    {
        V4L2_LOG("mmap failed: %s\n", strerror(errno));
        return false;
    }

    return true;
}

/* 取消映射缓冲数据指针 */
bool v4l2_unmapBuffer(int fd, struct v4l2_buffer* buffer, void* ptr)
{
    if(munmap(ptr, buffer->length) < 0)
    {
        V4L2_LOG("munmap failed: %s\n", strerror(errno));
        return false;
    }

    return true;
}

/* 将缓冲加入采集队列 */
bool v4l2_pushQueue(int fd, struct v4l2_buffer* buffer)
{
    if(ioctl(fd, VIDIOC_QBUF, buffer) < 0)
    {
        V4L2_LOG("ioctl VIDIOC_QBUF failed: %s\n", strerror(errno));
        return false;
    }

    return true;
}

/* 从采集队列中取出一帧 */
bool v4l2_popQueue(int fd, struct v4l2_buffer* buffer)
{
    if(ioctl(fd, VIDIOC_DQBUF, buffer) < 0)
    {
        V4L2_LOG("ioctl VIDIOC_QBUF failed: %s\n", strerror(errno));
        return false;
    }

    return true;
}

/* 开始采集 */
bool v4l2_start(int fd)
{
    enum v4l2_buf_type bufType = V4L2_BUF_TYPE_VIDEO_CAPTURE; 
    if(ioctl(fd, VIDIOC_STREAMON, &bufType) < 0)
    {
        fprintf(stderr, "ioctl VIDIOC_STREAMON failed: %s\n", strerror(errno));
        return false;
    }

    return true;
}

/* 停止采集 */
bool v4l2_stop(int fd)
{
    enum v4l2_buf_type bufType = V4L2_BUF_TYPE_VIDEO_CAPTURE; 
    if(ioctl(fd, VIDIOC_STREAMOFF, &bufType) < 0)
    {
        fprintf(stderr, "ioctl VIDIOC_STREAMOFF failed: %s\n", strerror(errno));
        return false;
    }

    return true;
}

/* 等待采集队列中有数据可读 */
bool v4l2_wait(int fd)
{
    struct pollfd fds[1];
    fds[0].fd = fd;
    fds[0].events = POLLIN | POLLPRI;
    return poll(fds, 1, -1) > 0;
}

/* 解压JPEG数据，转换成RGB格式 */
bool v4l2_jpegToRGB(const void* jpeg, size_t inSize, void** rgb, size_t* outSize)
{
    // 创建解压结构
    struct jpeg_decompress_struct cinfo;
    struct jpeg_error_mgr jerr;
    cinfo.err=jpeg_std_error(&jerr);
    jpeg_create_decompress(&cinfo);
    
    // 输入JPEG数据
    jpeg_mem_src(&cinfo, (const unsigned char*)(jpeg), inSize);
    
    // 解析header
    jpeg_read_header(&cinfo, false);
    jpeg_calc_output_dimensions(&cinfo);

    // 给RGB数据分配空间
    size_t rowSize = cinfo.output_components * cinfo.output_width;
    *outSize = rowSize * cinfo.output_height;
    *rgb = malloc(*outSize);

    // 行缓冲
    JSAMPARRAY buffer = cinfo.mem->alloc_sarray((j_common_ptr)(&cinfo), JPOOL_IMAGE, rowSize, 1);

    // 逐行解压
    jpeg_start_decompress(&cinfo);
    for(void* ptr = *rgb; cinfo.output_scanline < cinfo.output_height; ptr += rowSize)
    {
        jpeg_read_scanlines(&cinfo, buffer, 1);
        memcpy(ptr, buffer[0], rowSize);
    }

    // 结束
    jpeg_finish_decompress(&cinfo);
    jpeg_destroy_decompress(&cinfo);

    return true;
}