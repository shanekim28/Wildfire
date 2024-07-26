#define FNL_IMPL
#include "FastNoiseLite.h"
#include "firesim.h"
#include <stdlib.h>
#include <libc.h>

const int WF_DIRECTIONS[5][2] = {{0, 0}, {-1, 0}, {0, -1}, {1, 0}, {0, 1}};
int rowColToIndex(int row, int col, int width) { return (row * width + col); }

char isValidCell(int x, int y, int width, int height) {
    return (x >= 0 && x < width && y >= 0 && y < height);
}

uint8_t isTouchingPixel(uint8_t* map, int x, int y, int width, int height) {
    int directions[4][2] = {{-1, 0}, {0, -1}, {1, 0}, {0, 1}};
    for (int i = 0; i < 4; i++) {
        if (!isValidCell(x + directions[i][0], y + directions[i][1], width,
                         height))
            continue;

        if (map[(y + directions[i][1]) * width + (x + directions[i][0])]) {
            // FIXME There's a better way to do this but it's
            // not important right now
            if (i == 0) {
                return WF_RIGHT;
            } else if (i == 1) {
                return WF_UP;
            } else if (i == 2) {
                return WF_LEFT;
            } else {
                return WF_DOWN;
            }
        }
    }

    return 0;
}

uint8_t* upscale(uint8_t* map, int width, int height) {
    uint8_t* upscaled_map = calloc(width * height * 4, sizeof(uint8_t));
    for (int y = 0; y < width; y++) {
        for (int x = 0; x < height; x++) {
            int dir = map[y * width + x];
            switch (dir) {
            case WF_DOWN:
            case WF_UP:
                if (map[(y - 1) * width + x] == WF_LEFT ||
                    map[(y - 1) * width + x] == WF_RIGHT) {
                    upscaled_map[(y * 2 - 1) * width * 2 + (x * 2)] = dir;
                }
                upscaled_map[(y * 2) * width * 2 + (x * 2)] = dir;
                upscaled_map[(y * 2 + 1) * width * 2 + (x * 2)] = dir;
                break;
            case WF_LEFT:
            case WF_RIGHT:
                if (map[y * width + x - 1] == WF_DOWN ||
                    map[y * width + x - 1] == WF_UP) {
                    upscaled_map[(y * 2) * width * 2 + (x * 2 - 1)] = dir;
                }
                upscaled_map[(y * 2) * width * 2 + (x * 2)] = dir;
                upscaled_map[(y * 2) * width * 2 + (x * 2) + 1] = dir;
                break;
            case 5:
                upscaled_map[(y * 2) * width * 2 + (x * 2)] = dir;
                upscaled_map[(y * 2) * width * 2 + (x * 2) + 1] = dir;
                upscaled_map[(y * 2 + 1) * width * 2 + (x * 2)] = dir;
                upscaled_map[(y * 2 + 1) * width * 2 + (x * 2) + 1] = dir;
                break;
            default:
                break;
            }
        }
    }

    return upscaled_map;
}

void jitter(uint8_t** input_map, int width, int height) {
    uint8_t* map = *input_map;
    uint8_t* jitter_map = calloc(width * height, sizeof(uint8_t));
    fnl_state noise = fnlCreateState();
    noise.seed = 1337;
    noise.noise_type = FNL_NOISE_OPENSIMPLEX2;
    noise.frequency = 0.05f;
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int dir = map[y * width + x];
            // Jitter this pixel if this is the same as the next one
            if (map[(y + WF_DIRECTIONS[dir][1]) * width + x +
                    WF_DIRECTIONS[dir][0]] == dir) {
                float jitter_data = fnlGetNoise2D(&noise, x, y);
                int jitter = (int)(jitter_data * 3);
                printf("%d\n", jitter);

                if (dir == WF_LEFT || dir == WF_RIGHT) {
                    jitter_map[(y + jitter) * width + x] = dir;
                } else if (dir == WF_UP || dir == WF_DOWN) {
                    jitter_map[y * width + (x + jitter)] = dir;
                } else {
                    jitter_map[y * width + x] = dir;
                }
            }
        }
    }

    free(map);
    *input_map = jitter_map;
}

/// @brief Generates a heightmap using Diffusion Limited Aggregation
/// (https://www.youtube.com/watch?v=gsJHzBTPG0Y)
/// @param height_map height map to generate
/// @param width Width of the height map
/// @param height Height of the height map
/// @param seed Seed for random number generator
void generate_heightmap(float* height_map, int width, int height, int seed) {
    srand(seed);

    int base_map_width = 50;
    int num_points = 100;
    uint8_t* base_map =
        calloc(base_map_width * base_map_width, sizeof(uint8_t));

    for (int i = 0; i < num_points; i++) {
        // Get random unoccupied pixel
        int x = rand() % base_map_width;
        int y = rand() % base_map_width;
        // Check if occupied
        while (base_map[y * base_map_width + x] > 0) {
            x = rand() % base_map_width;
            y = rand() % base_map_width;
        }

        // Don't move first pixel
        if (i == 0) {
            base_map[y * base_map_width + x] = 5; // Set pixel
            continue;
        }

        // Move pixel until it touches a neighbor
        int direction = 0;
        while (!(direction = isTouchingPixel(base_map, x, y, base_map_width,
                                             base_map_width))) {
            int directions[4][2] = {{-1, 0}, {0, -1}, {1, 0}, {0, 1}};
            int randIndex = rand() % 4;
            int dirX = directions[randIndex][0];
            int dirY = directions[randIndex][1];

            if (!isValidCell(x + dirX, y + dirY, base_map_width,
                             base_map_width)) {
                continue;
            }

            x += dirX;
            y += dirY;
        }

        base_map[y * base_map_width + x] = direction;
    }

    int scale = 8;
    uint8_t* new_map = base_map;
    for (int i = 1; i < scale; i *= 2) {
        uint8_t* temp =
            upscale(new_map, base_map_width * i, base_map_width * i);
        free(new_map);
        new_map = temp;
    }

    // jitter(&new_map, base_map_width * scale, base_map_width * scale);

    // Repeat until desired detail level is reached:
    // 1. Scale + blur (version A)
    // 2. Upscale (version B)
    // 3. Add detail to version B
    // 4. Apply details to version A

    // Draw to height map
    for (int y = 0; y < base_map_width * scale; y++) {
        for (int x = 0; x < base_map_width * scale; x++) {
            int idx = y * base_map_width * scale + x;
            height_map[y * width + x] = new_map[idx];
        }
    }
}

// Generate a fire map with specified width and height
void init_fire_sim(Simulation* sim, int width, int height) {
    // TODO Initialize simulation
    // Generate topography
    sim->height_map = malloc(width * height * sizeof(float));
    generate_heightmap(sim->height_map, width, height, 1337);
    //  Generate vegetation map
    //  sim->vegetation_map = malloc(width * height * sizeof(float));
    sim->wind_map = calloc(width * height, sizeof(Vector2));
    sim->temperature_map = malloc(width * height * sizeof(float));
    for (int i = 0; i < width * height; i++) {
        sim->temperature_map[i] = -1;
    }
    sim->width = width;
    sim->height = height;

    sim->temperature_map[0] = 1;
}

void advance_fire_sim(Simulation* sim) {
    // Save state of current board
    float* new_temperature_map =
        malloc(sim->width * sim->height * sizeof(float));
    memcpy(new_temperature_map, sim->temperature_map,
           sim->width * sim->height * sizeof(float));

    for (int i = 0; i < sim->width * sim->height; i++) {
        // Cell is not on fire, don't need to simulate
        if (sim->temperature_map[i] <= 0) {
            continue;
        }

        float ignition_probability = BASE_IGNITION_PROBABILITY;

        // Current cell is burning, update state in new map
        new_temperature_map[i] -= 0.05f;
        new_temperature_map[i] = MAX(0, new_temperature_map[i]);

        int row = i / sim->width;
        int col = i % sim->width;
        // TODO Cellular automata
        // Check neighboring cells
        for (int neighborRow = -1; neighborRow <= 1; neighborRow++) {
            for (int neighborCol = -1; neighborCol <= 1; neighborCol++) {
                // Ignore current cell
                if (neighborRow == 0 && neighborRow == neighborCol)
                    continue;

                // Bounds check
                if (!isValidCell(col + neighborCol, row + neighborRow,
                                 sim->width, sim->height))
                    continue;

                // Ignore burned/burning cells
                int neighborIndex = rowColToIndex(
                    row + neighborRow, col + neighborCol, sim->width);
                if (sim->temperature_map[neighborIndex] >= 0 ||
                    new_temperature_map[neighborIndex] >= 0)
                    continue;

                // Ignite cell
                float rand = (random() % 100) / 100.0f;
                if (rand < ignition_probability)
                    new_temperature_map[neighborIndex] = 1;
            }
        }
    }

    // Swap buffers
    free(sim->temperature_map);
    sim->temperature_map = new_temperature_map;
}

void dispose(Simulation* sim) {
    // free(sim->height_map);
    // free(sim->vegetation_map);
    free(sim->temperature_map);
    free(sim);
}