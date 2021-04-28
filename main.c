#include <stdint.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <SDL2/SDL.h>
#include "v4l2.h"

#define VIDEO_DEV "/dev/video0"
#define BUFFER_COUNT 5

int main()
{
    /* 打开设备 */
    int fd = open(VIDEO_DEV, O_RDWR);
    if(fd < 0)
    {
        fprintf(stderr, "open %s failed: %s\n", VIDEO_DEV, strerror(errno));
        return EXIT_FAILURE;
    }

    /* 设置像素格式 */
    if(!v4l2_testPixFormat(fd, V4L2_PIX_FMT_MJPEG))
    {
        fprintf(stderr, "YUYV is unsupported\n");
        return EXIT_FAILURE;
    }
    v4l2_setPixFormat(fd, V4L2_PIX_FMT_MJPEG);

    /* 读取分辨率 */
    uint32_t w, h;
    v4l2_getSize(fd, &w, &h);
    printf("size: %u x %u\n", w, h);

    /* 读取帧率 */
    uint32_t fps;
    v4l2_getFPS(fd, &fps);
    printf("FPS: %u\n", fps);

    /* 申请5帧缓冲 */
    v4l2_requestBuffer(fd, BUFFER_COUNT);

    /* 获取缓冲数据指针 */
    struct v4l2_buffer buffer[BUFFER_COUNT];
    void* bufPtr[BUFFER_COUNT];
    for(uint32_t i = 0; i < BUFFER_COUNT; i++)
    {
        v4l2_getBuffer(fd, i, &(buffer[i]));
        v4l2_mapBuffer(fd, &(buffer[i]), &(bufPtr[i]));
    }

    // 创建窗口
    SDL_Window* window = SDL_CreateWindow(
        "Camera", 
        SDL_WINDOWPOS_UNDEFINED, 
        SDL_WINDOWPOS_UNDEFINED,
        w, h, SDL_WINDOW_SHOWN);

    // 创建渲染器
    SDL_Renderer* renderer = SDL_CreateRenderer(
        window, 
        -1, 
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_TARGETTEXTURE);

    // 创建纹理
    SDL_Texture* texture = SDL_CreateTexture(
        renderer, 
        SDL_PIXELFORMAT_RGB24, 
        SDL_TEXTUREACCESS_STREAMING,
        w, h
    );

    // 将5个缓冲加入采集队列
    for(uint32_t i = 0; i < BUFFER_COUNT; i++)
        v4l2_pushQueue(fd, &(buffer[i]));

    // 开始采集
    v4l2_start(fd);

    // 矩形范围
    SDL_Rect rect = {0, 0, w, h};

    // 事件
    SDL_Event event;
    uint32_t index = 0;
    bool running = true;
    while(running)
    {
        // 处理事件
        while(SDL_PollEvent(&event) > 0)
        {
            if(event.type == SDL_QUIT)
            {
                running = false;
            }
        }

        // 等待采集完成
        if(!v4l2_wait(fd))
            continue;

        // 读取一帧缓冲
        v4l2_popQueue(fd, &(buffer[index]));

        // JPEG 转 RGB
        void* rgb = NULL;
        size_t outSize = 0;
        v4l2_jpegToRGB(bufPtr[index], buffer[index].bytesused, &rgb, &outSize);

        // 显示
        if(SDL_UpdateTexture(texture, NULL, rgb, w * 3) < 0)
        {
            SDL_Log("SDL_UpdateTexture failed: %s\n", SDL_GetError());
        }
        free(rgb);
        SDL_RenderCopy(renderer, texture, NULL, NULL);
        SDL_RenderPresent(renderer);

        // 将缓冲再次加入采集队列
        v4l2_pushQueue(fd, &(buffer[index]));
        index = (index + 1) % BUFFER_COUNT;
    }

    // 停止采集
    v4l2_stop(fd);

    // 释放资源
    SDL_DestroyTexture(texture);
    SDL_DestroyWindow(window);
    SDL_DestroyRenderer(renderer);

    for(uint32_t i; i < BUFFER_COUNT; i++)
    {
        v4l2_unmapBuffer(fd, &(buffer[i]), bufPtr[i]);
    }
    close(fd);

    SDL_Quit();
    return EXIT_SUCCESS;
}