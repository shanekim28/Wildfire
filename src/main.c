#include <SDL3/SDL.h>
#include <GL/glew.h>
#include <stdlib.h>
#include <libc.h>
#include "firesim.h"
#include <math.h>

#define PI 3.14159265358979323846

char paused = 1;
int draw_mode = 0;

void run(Simulation* sim);
void simulate(Simulation* sim);
void render(SDL_Renderer* renderer, Simulation* sim);

int main(int argc, char** argv) {
    Simulation sim = {0};
    init_fire_sim(&sim, 800, 600);
    run(&sim);
    return 0;
}

void simulate(Simulation* sim) {
    advance_wind_sim(sim);
    advance_fire_sim(sim);
}

void render(SDL_Renderer* renderer, Simulation* sim) {
    SDL_Surface* surface =
        SDL_CreateSurface(sim->width, sim->height, SDL_PIXELFORMAT_RGBA8888);

    int pitch = surface->pitch;
    int bytes_per_pixel = surface->format->bytes_per_pixel;

    uint8_t* pixels = (uint8_t*)surface->pixels;
    for (int y = 0; y < sim->height; y++) {
        for (int x = 0; x < sim->width; x++) {
            int index = y * pitch + x * bytes_per_pixel;
            if (draw_mode == 0) {
                pixels[index] = 255;
                if (sim->temperature_map[y * sim->width + x] >= 0) {
                    pixels[index + 1] = 0;
                    pixels[index + 2] = 0;
                    pixels[index + 3] =
                        255 * sim->temperature_map[y * sim->width + x];
                } else {
                    pixels[index + 1] = 0;
                    pixels[index + 2] = 154;
                    pixels[index + 3] = 64;
                }
            } else if (draw_mode == 1) {
                int dir = sim->height_map[y * sim->width + x];
                pixels[index] = 255;
                if (dir > 0) {
                    memset(pixels + index + 1, 255, 3);
                } else {
                    memset(pixels + index + 1, 0, 3);
                }
                /*
                pixels[index] = 255;
                int dir = sim->height_map[y * sim->width + x];
                if (dir > 0) {
                    memset(pixels + index + 1, 255, 3);
                } else {
                    pixels[index + 1] = 0;
                    pixels[index + 2] = 0;
                    pixels[index + 3] = 0;
                }
                */
            } else if (draw_mode == 2) {
                if (x % 20 != 0 || y % 20 != 0) {
                    continue;
                }

                Vector2 wind = sim->wind_map[y * sim->width + x];
                float magnitude = sqrtf(powf(wind.x, 2) + powf(wind.y, 2));
                SDL_SetRenderDrawColor(renderer, 112, 0, 255, 255);
                SDL_FRect* rect = malloc(sizeof(SDL_FRect));
                rect->x = x - 1;
                rect->y = y - 1;
                rect->w = 3;
                rect->h = 3;
                SDL_RenderRect(renderer, rect);
                SDL_RenderLine(renderer, x, y, x + wind.x / magnitude * 10,
                               y + wind.y / magnitude * 10);
            }
        }
    }

    SDL_RenderTexture(renderer, SDL_CreateTextureFromSurface(renderer, surface),
                      NULL, NULL);
    SDL_RenderPresent(renderer);
}

void run(Simulation* sim) {
    SDL_Window* window = NULL;
    if (SDL_InitSubSystem(SDL_INIT_VIDEO)) {
        SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, SDL_GetError());
    }
    window = SDL_CreateWindow("Wildfire", 800, 600,
                              SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL);

    if (!window) {
        SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, SDL_GetError());
    }

    char isRunning = 1;

    SDL_Renderer* renderer = NULL;
    renderer = SDL_CreateRenderer(window, NULL);
    while (isRunning) {
        SDL_Event event;

        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_KEY_DOWN) {
                switch (event.key.key) {
                case SDLK_1:
                    draw_mode = 0;
                    break;
                case SDLK_2:
                    draw_mode = 1;
                    break;
                case SDLK_3:
                    draw_mode = 2;
                    break;
                case SDLK_4:
                    draw_mode = 3;
                    break;
                case SDLK_SPACE:
                    paused = 1 - paused;
                    break;
                default:
                    break;
                }
            }
            if (event.type == SDL_EVENT_QUIT) {
                isRunning = 0;
            }
        }

        if (!paused)
            simulate(sim);
        render(renderer, sim);
    }

    SDL_DestroyWindow(window);
    SDL_QuitSubSystem(SDL_INIT_VIDEO);
}