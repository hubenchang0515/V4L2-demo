#ifndef V4L2_H
#define V4L2_H

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <linux/videodev2.h> // libv4l-dev

#ifndef DEBUG
    #define V4L2_LOG(...)
#else
    #define V4L2_LOG(...) printf(__VA_ARGS__)
#endif

/* 判断设备是否支持指定的像素格式 */
bool v4l2_testPixFormat(int fd, uint32_t fmt);

/* 设置设备的像素格式 */
bool v4l2_setFormat(int fd, uint32_t fmt);

/* 获取像素格式 */
bool v4l2_getFormat(int fd, uint32_t* fmt);

/* 设置像素格式 */
bool v4l2_setPixFormat(int fd, uint32_t fmt);

/* 获取分辨率 */
bool v4l2_getSize(int fd, uint32_t* width, uint32_t* height);

/* 设置分辨率 */
bool v4l2_setSize(int fd, uint32_t width, uint32_t height);

/* 设置采集范围 */
bool v4l2_setArea(int fd, int x, int y, int width, int height);

/* 获取帧率 */
bool v4l2_getFPS(int fd, uint32_t* fps);

/* 设置帧率 */
bool v4l2_setFPS(int fd, uint32_t fps);

/* 申请指定数量的帧缓冲 */
bool v4l2_requestBuffer(int fd, uint32_t count);

/* 获取帧缓冲 */
bool v4l2_getBuffer(int fd, uint32_t index, struct v4l2_buffer* buffer);

/* 映射缓冲数据指针 */
bool v4l2_mapBuffer(int fd, struct v4l2_buffer* buffer, void** ptr);

/* 取消映射缓冲数据指针 */
bool v4l2_unmapBuffer(struct v4l2_buffer* buffer, void* ptr);

/* 将帧缓冲加入采集队列 */
bool v4l2_pushQueue(int fd, struct v4l2_buffer* buffer);

/* 从采集队列中取出一帧 */
bool v4l2_popQueue(int fd, struct v4l2_buffer* buffer);

/* 开始采集 */
bool v4l2_start(int fd);

/* 停止采集 */
bool v4l2_stop(int fd);


#endif