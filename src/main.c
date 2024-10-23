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
    WATERMAP = 1 << 1,
};

char paused = 1;
enum DrawMode draw_mode = DEFAULT;

float zoom = 1.0f;
int offsetX = (800 - 300) / 2, offsetY = (600 - 300) / 2;

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
                SDL_SetRenderDrawColor(renderer, 10, 87, 17, 255);
            }

            SDL_RenderFillRect(renderer, &rect);

            if (draw_mode & HEIGHTMAP) {
                float val = sim->map->height_map[y * sim->width + x];
                SDL_SetRenderDrawColor(renderer, 255 * (val + 1) / 2.0f, 255 * (val + 1) / 2.0f, 255 * (val + 1) / 2.0f, 255);

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
                        printf("%f\n", zoom);
                        break;
                    case SDLK_E:
                        zoom *= 2.0f;
                        printf("%f\n", zoom);
                        break;
                    case SDLK_1:
                        draw_mode = DEFAULT;
                        break;
                    case SDLK_2:
                        draw_mode = HEIGHTMAP;
                        break;
                    case SDLK_3:
                        draw_mode |= WATERMAP;
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
            nk_layout_row_static(ctx, 30, 80, 1);
            if (nk_button_label(ctx, "Regenerate")) {
                sim->map->seed = seed;
                generate_map(sim->map);
                printf("Regenerating map\n");
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
