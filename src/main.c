#include <SDL3/SDL.h>
#include <GL/glew.h>
#include <stdlib.h>
#include <libc.h>
#include "firesim.h"
#include <math.h>

#define SDL_ENABLE_OLD_NAMES

#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#define NK_IMPLEMENTATION
#define NK_SDL_RENDERER_IMPLEMENTATION
#include "nuklear.h"
#include "updated_nuklear_sdl_renderer.h"

#define PI 3.14159265358979323846

enum DrawMode {
    DEFAULT = 0,
    HEIGHTMAP = 1 << 0,
    TERRAIN = 1 << 1,
    WATERMAP = 1 << 2,
};

char paused = 1;
enum DrawMode draw_mode = DEFAULT;

float zoom = 2.0f;
int offsetX = (800 - 300) / 2, offsetY = (600 - 300) / 2;

void run(Simulation* sim);
void simulate(Simulation* sim);
void render(SDL_Renderer* renderer, Simulation* sim);

int main(int argc, char** argv) {
    Simulation sim = {0};
    init_fire_sim(&sim, 256, 256);
    run(&sim);
    return 0;
}

void simulate(Simulation* sim) { advance_fire_sim(sim); }

SDL_Color whittaker[] = {
    { .r = 86, .g = 139, .b = 112, .a = 255  },
    { .r = 13, .g = 66, .b = 27, .a = 255  },
    { .r = 115, .g = 115, .b = 115, .a = 255  },
    { .r = 115, .g = 115, .b = 115, .a = 255  },
    { .r = 248, .g = 248, .b = 248, .a = 255  },
};
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
                SDL_SetRenderDrawColor(renderer, 10, 87, 17, 255);
            }

            SDL_RenderFillRect(renderer, &rect);

            if (draw_mode & HEIGHTMAP) {
                float val = sim->map->height_map[y * sim->width + x];
                SDL_SetRenderDrawColor(renderer, 255 * val, 255 * val, 255 * val, 255);

                SDL_FRect rect2 = {
                    (int)screenX1,
                    (int)screenY1,
                    (int)zoom,
                    (int)zoom
                };
                SDL_RenderFillRect(renderer, &rect2);
            }

            if (draw_mode & TERRAIN) {
                float val = sim->map->height_map[y * sim->width + x];
                float t = val;

                int size = sizeof(whittaker) / sizeof(whittaker[0]);

                int val1idx = (val * (size - 1));
                int val2idx = ceil(val * (size - 1));

                if (val1idx >= 0 && val1idx < size && val2idx >= 0 && val2idx < size) {
                    SDL_Color v1 = whittaker[val1idx];
                    SDL_Color v2 = whittaker[val2idx];
                    float r = (1 - t) * v1.r + t * v2.r;
                    float g = (1 - t) * v1.g + t * v2.g;
                    float b = (1 - t) * v1.b + t * v2.b;
                    
                    SDL_SetRenderDrawColor(renderer, r, g, b, 255);
                }


                SDL_FRect rect2 = {
                    (int)screenX1,
                    (int)screenY1,
                    (int)zoom,
                    (int)zoom
                };
                SDL_RenderFillRect(renderer, &rect2);
            }

            if (draw_mode & WATERMAP) {
                float val = sim->map->water_map[y * sim->width + x];
                if (val > 0)
                    SDL_SetRenderDrawColor(renderer, 59, 150, 235, 255);

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

void run(Simulation* sim) {
    SDL_Window* window = NULL;
    SDL_Renderer* renderer = NULL;

    int flags = 0;
    char isRunning = 1;
    float font_scale = 1;

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, SDL_GetError());
    }

    window = SDL_CreateWindow("Wildfire", 800, 600,
            SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY);
    if (!window) {
        SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, SDL_GetError());
    }

    renderer = SDL_CreateRenderer(window, NULL);

    {
        int render_w, render_h;
        int window_w, window_h;
        float scale_x, scale_y;
        SDL_GetRenderOutputSize(renderer, &render_w, &render_h);
        SDL_GetWindowSize(window, &window_w, &window_h);
        scale_x = (float)(render_w) / (float)(window_w);
        scale_y = (float)(render_h) / (float)(window_h);
        SDL_SetRenderScale(renderer, scale_x, scale_y);
        font_scale = scale_y;
    }

    struct nk_context* ctx;
    ctx = nk_sdl_init(window, renderer);

    {
        struct nk_font_atlas* atlas;
        struct nk_font_config config = nk_font_config(0);
        struct nk_font* font;
        nk_sdl_font_stash_begin(&atlas);
        font = nk_font_atlas_add_default(atlas, 13 * font_scale, &config);
        //struct nk_font* font = nk_font_atlas_add_from_file(atlas, "fonts/IBM_Plex_Mono/IBMPlexMono-Regular.ttf", 13, 0);
        nk_sdl_font_stash_end();

        font->handle.height /= font_scale;
        nk_style_set_font(ctx, &font->handle);
    }

    struct nk_colorf bg;
    bg.r = 0.10f, bg.g = 0.18f, bg.b = 0.24f, bg.a = 1.0f;
    while (isRunning) {
        SDL_RenderClear(renderer);
        SDL_Event event;
        nk_input_begin(ctx);

        while (SDL_PollEvent(&event)) {
            nk_sdl_handle_event(&event);
            if (event.type == SDL_EVENT_KEY_DOWN) {
                switch (event.key.key) {
                    case SDLK_Q:
                        zoom /= 2.0f;
                        break;
                    case SDLK_E:
                        zoom *= 2.0f;
                        break;
                    case SDLK_1:
                        draw_mode = DEFAULT;
                        break;
                    case SDLK_2:
                        draw_mode = HEIGHTMAP;
                        break;
                    case SDLK_3:
                        draw_mode = TERRAIN;
                        break;
                    case SDLK_SPACE:
                        paused = 1 - paused;
                        break;
                    case SDLK_ESCAPE:
                        isRunning = 0;
                        break;
                }
            }
            if (event.type == SDL_EVENT_QUIT) {
                isRunning = 0;
            }
        }

        const bool* keystates = SDL_GetKeyboardState(NULL);
        if (keystates[SDL_SCANCODE_H]) {
            offsetX += 10;
        }
        if (keystates[SDL_SCANCODE_J]) {
            offsetY -= 10;
        }
        if (keystates[SDL_SCANCODE_K]) {
            offsetY += 10;
        }
        if (keystates[SDL_SCANCODE_L]) {
            offsetX -= 10;
        }

        nk_sdl_handle_grab();
        nk_input_end(ctx);

        if (!paused)
            simulate(sim);

        render(renderer, sim);

        if (nk_begin(ctx, "Settings", nk_rect(50, 50, 230, 250), NK_WINDOW_BORDER | NK_WINDOW_MOVABLE)) {
            enum {EASY, HARD};
            static int op = EASY;
            static int seed = 20;

            if (nk_tree_push(ctx, NK_TREE_TAB, "Resources", NK_MINIMIZED)) {
                nk_layout_row_dynamic(ctx, 80, 4);
                nk_button_label(ctx, "CRW");
                nk_button_label(ctx, "HVY");
                nk_button_label(ctx, "AIR");
                nk_button_label(ctx, "SUP");
                nk_tree_pop(ctx);
            }
            nk_layout_row_dynamic(ctx, 25, 1);
            nk_property_int(ctx, "Seed:", INT_MIN, &seed, INT_MAX, 1, 1);
            nk_layout_row_static(ctx, 30, 80, 2);
            if (nk_button_label(ctx, "Regenerate")) {
                sim->map->seed = seed;
                generate_map(sim->map);
                printf("Regenerating map\n");
            }
            if (nk_button_label(ctx, "Save")) {
                generate_map(sim->map);

                SDL_Surface* surface = SDL_CreateSurface(sim->map->width, sim->map->height, SDL_PIXELFORMAT_RGBA8888);
                Uint32* pixelData = (Uint32*)surface->pixels;
                for (int y = 0; y < sim->map->height; y++) {
                    for (int x = 0; x < sim->map->width; x++) {
                        Uint8 color = (Uint8)(sim->map->height_map[y * sim->map->width + x] * 255);
                        Uint32 grayscale = (color << 24) | (color << 16) | (color << 8) | 0xFF;
                        pixelData[y * sim->map->width + x] = grayscale;
                    }
                }
                SDL_SaveBMP(surface, "output.bmp");
                printf("Successfully saved map\n");
            }
        }
        nk_end(ctx);

        nk_sdl_render(NK_ANTI_ALIASING_ON);
        SDL_RenderPresent(renderer);
    }

    nk_sdl_shutdown();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_QuitSubSystem(SDL_INIT_VIDEO);
    SDL_Quit();
}
