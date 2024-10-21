#include <SDL3/SDL.h>
#include <GL/glew.h>
#include <stdlib.h>
#include <libc.h>
#include "firesim.h"
#include <math.h>

#define PI 3.14159265358979323846

enum DrawMode {
    DEFAULT = 0,
    HEIGHTMAP = 1
};

char paused = 1;
enum DrawMode draw_mode = DEFAULT;

float zoom = 1.0f;
int offsetX = 0, offsetY = 0;

void run(Simulation* sim);
void simulate(Simulation* sim);
void render(SDL_Renderer* renderer, Simulation* sim);

int main(int argc, char** argv) {
    Simulation sim = {0};
    init_fire_sim(&sim, 300, 300);
    run(&sim);
    return 0;
}

void simulate(Simulation* sim) { advance_fire_sim(sim); }

void render(SDL_Renderer* renderer, Simulation* sim) {
    SDL_SetRenderDrawColor(renderer, 25, 23, 36, 255);
    SDL_RenderClear(renderer);
    for (int y = 0; y < sim->height; y++) {
        for (int x = 0; x < sim->width; x++) {
            float screenX1 =  (x * zoom) + offsetX;
            float screenY1 = (y * zoom) + offsetY;

            SDL_FRect rect = { 
                (int)screenX1, 
                (int)screenY1, 
                (int)zoom,
                (int)zoom
            };
            if (sim->temperature_map[y * sim->width + x] >= 0) {
                SDL_SetRenderDrawColor(renderer, 255 * sim->temperature_map[y * sim->width + x], 0, 0, 255);
            } else {
                SDL_SetRenderDrawColor(renderer, 64, 154, 0, 255);
            }

            SDL_RenderFillRect(renderer, &rect);

            if (draw_mode & HEIGHTMAP) {
                if (sim->height_map[y * sim->width + x] > 0) {
                    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

                    SDL_FRect rect2 = {
                        (int)screenX1,
                        (int)screenY1,
                        (int)zoom,
                        (int)zoom
                    };
                    SDL_RenderFillRect(renderer, &rect2);
                }                  
            }
        }
    }

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
                    case SDLK_q:
                        zoom /= 2.0f;
                        printf("%f\n", zoom);
                        break;
                    case SDLK_e:
                        zoom *= 2.0f;
                        printf("%f\n", zoom);
                        break;
                    case SDLK_1:
                        draw_mode = DEFAULT;
                        break;
                    case SDLK_2:
                        draw_mode = HEIGHTMAP;
                        break;
                    case SDLK_SPACE:
                        paused = 1 - paused;
                        break;
                    case SDLK_ESCAPE:
                        isRunning = 0;
                        break;
                    case SDLK_h:
                        offsetX += 20;
                        break;
                    case SDLK_j:
                        offsetY -= 20;
                        break;
                    case SDLK_k:
                        offsetY += 20;
                        break;
                    case SDLK_l:
                        offsetX -= 20;
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
