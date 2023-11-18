#include <SDL2/SDL.h>
#include <SDL2/SDL_audio.h>
#include <iostream>
#include <vector>
#include <cmath>
#include <mutex>

const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;
const int AUDIO_BUFFER_SIZE = 640;

SDL_Renderer* renderer = nullptr;
SDL_Window* window = nullptr;

std::vector<double> dbBuffer(WINDOW_WIDTH, 0);
Uint32 dbBufferPos = 0;
std::mutex dbBufferMutex;

void audioCallback(void *userdata, Uint8 *stream, int len)
{
    Sint16 *stream16 = (Sint16 *)stream;
    // https://stackoverflow.com/questions/535246/sound-pressure-display-for-wave-pcm-data
    double sum = 0;
    for (int i = 0; i < len/2; ++i) {
        sum += stream16[i] * stream16[i];
    }
    double average = sum / len / 2;
    std::lock_guard<std::mutex> lock(dbBufferMutex);
    auto db = 10 * std::log10(average);
    if (db < 0.0) {
        db = 0;
    }
    dbBuffer[dbBufferPos] = db;
    dbBufferPos = (dbBufferPos + 1) % WINDOW_WIDTH;
    SDL_Log("average = %f, db = %f, len = %d", average, db, len);
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
                break;
            }
        }

        // 渲染波形图（示例：用蓝色线表示波形）
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        uint64_t now = SDL_GetPerformanceCounter();
        std::lock_guard<std::mutex> lock(dbBufferMutex);
        int x0 = 0, y0 = 0;
        for (int i = 0; i < WINDOW_WIDTH; ++i) {
            int bi = (dbBufferPos + i + 1) % WINDOW_WIDTH;
            int y = WINDOW_HEIGHT / 2 - dbBuffer[bi];
            SDL_RenderDrawLine(renderer, x0, y0, i, y);
            x0 = i;
            y0 = y;
        }

        SDL_RenderPresent(renderer);
    }

    // 关闭SDL
    SDL_CloseAudio();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
