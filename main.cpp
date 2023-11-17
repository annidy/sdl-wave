#include <SDL2/SDL.h>
#include <SDL2/SDL_audio.h>
#include <iostream>
#include <vector>

const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;
const int AUDIO_BUFFER_SIZE = 1024;

SDL_Renderer* renderer = nullptr;
SDL_Window* window = nullptr;

std::vector<Sint16> audioBuffer(AUDIO_BUFFER_SIZE, 0);
Uint32 audioBufferPosition = 0;

void audioCallback(void *userdata, Uint8 *stream, int len)
{
    memcpy(&audioBuffer[0], stream, len);
}

int main(int argc, char* argv[]) {
    // 初始化SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0) {
        std::cerr << "SDL初始化失败: " << SDL_GetError() << std::endl;
        return 1;
    }

    // 创建窗口和渲染器
    window = SDL_CreateWindow("实时波形图", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN);
    if (window == nullptr) {
        std::cerr << "窗口创建失败: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (renderer == nullptr) {
        std::cerr << "渲染器创建失败: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }


    const char *deviceName = SDL_GetAudioDeviceName(0, SDL_TRUE);
    printf("%d - %s\n", 0, deviceName);

    // 配置音频规格
    SDL_AudioSpec desiredSpec, obtainedSpec;
    desiredSpec.freq = 16000;
    desiredSpec.format = AUDIO_S16SYS;
    desiredSpec.channels = 1;
    desiredSpec.samples = AUDIO_BUFFER_SIZE;
    desiredSpec.callback = audioCallback;

    // Open recording device
    int recordingDeviceId = SDL_OpenAudioDevice(SDL_GetAudioDeviceName(0, SDL_TRUE), SDL_TRUE, &desiredSpec, &obtainedSpec, SDL_AUDIO_ALLOW_FORMAT_CHANGE);

    // 打开音频设备
    if (recordingDeviceId < 0) {
            std::cerr << "无法打开音频设备: " << SDL_GetError() << std::endl;
            SDL_DestroyRenderer(renderer);
            SDL_DestroyWindow(window);
            SDL_Quit();
            return 1;
        }

    // 启动音频回调
    SDL_PauseAudioDevice(recordingDeviceId, SDL_FALSE);

    // 主循环
    bool quit = false;
    SDL_Event e;
    while (!quit) {
        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT) {
                quit = true;
            }
        }

        // 渲染波形图（示例：用蓝色线表示波形）
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        int x0 = 0, y0 = 0;
        for (int i = 0; i < WINDOW_WIDTH; ++i) {
            int ai = AUDIO_BUFFER_SIZE / WINDOW_WIDTH * i;
            int y = WINDOW_HEIGHT / 2 + audioBuffer[ai] / double(SDL_MAX_SINT16) * WINDOW_HEIGHT / 2;
            SDL_RenderDrawLine(renderer, x0, y0, i, y);
            x0 = i;
            y0 = y;
        }

        SDL_RenderPresent(renderer);

        // 重置音频缓冲区位置
        audioBufferPosition = 0;
    }

    // 关闭SDL
    SDL_CloseAudio();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
