#define FNL_IMPL
#include "firesim.h"
#include <stdlib.h>
#include <libc.h>

int rowColToIndex(int row, int col, int width) { return (row * width + col); }

char isValidCell(int x, int y, int width, int height) {
    return (x >= 0 && x < width && y >= 0 && y < height);
}

/// @brief Generates a heightmap using Diffusion Limited Aggregation
/// (https://www.youtube.com/watch?v=gsJHzBTPG0Y)
/// @param height_map height map to generate
/// @param width Width of the height map
/// @param height Height of the height map
/// @param seed Seed for random number generator
void generate_heightmap(float* height_map, int width, int height, int seed) {}

fnl_state generate_windmap(Vector2* wind_map, int width, int height, int seed) {
    fnl_state noise = fnlCreateState();
    noise.noise_type = FNL_NOISE_OPENSIMPLEX2;
    noise.seed = seed;
    noise.octaves = 2;
    noise.frequency = 0.002f;
    noise.weighted_strength = 1.0f;

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            float delta = 0.001f;
            // Sample barometric pressure at point
            float p = fnlGetNoise3D(&noise, x, y, 0);
            // Calculate gradient in x, y direction
            float p_dx = fnlGetNoise3D(&noise, x + delta, y, 0);
            float p_dy = fnlGetNoise3D(&noise, x, y + delta, 0);
            Vector2 wind_vector = {.x = (p - p_dx) * (1.0f / delta),
                                   .y = (p - p_dy) * (1.0f / delta)};

            wind_map[rowColToIndex(y, x, width)] = wind_vector;
        }
    }

    return noise;
}

void advance_wind_sim(Simulation* sim) {
    for (int y = 0; y < sim->height; y++) {
        for (int x = 0; x < sim->width; x++) {
            float delta = 0.001f;
            // Sample barometric pressure at point
            float p = fnlGetNoise3D(&sim->noise, x, y, sim->t);
            // Calculate gradient in x, y direction
            float p_dx = fnlGetNoise3D(&sim->noise, x + delta, y, sim->t);
            float p_dy = fnlGetNoise3D(&sim->noise, x, y + delta, sim->t);
            Vector2 wind_vector = {.x = (p - p_dx) * (1.0f / delta),
                                   .y = (p - p_dy) * (1.0f / delta)};

            sim->wind_map[rowColToIndex(y, x, sim->width)] = wind_vector;
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
    sim->noise = generate_windmap(sim->wind_map, width, height, 1337);
    sim->temperature_map = malloc(width * height * sizeof(float));
    for (int i = 0; i < width * height; i++) {
        sim->temperature_map[i] = -1;
    }
    sim->width = width;
    sim->height = height;

    sim->temperature_map[400 * height + 400] = 1;
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
                Vector2 wind = sim->wind_map[neighborIndex];
                float theta = atan2(neighborCol - wind.y, neighborRow - wind.x);
                float V = sqrt(wind.x * wind.x + wind.y * wind.y) * 250;
                float p_0 = BASE_IGNITION_PROBABILITY;
                float p_wind = exp(V * (C1 + C2 * cos(theta) - 1));
                float p_burn = p_0 * p_wind;
                float rand = (random() % 100) / 100.0f;
                if (rand < p_burn)
                    new_temperature_map[neighborIndex] = 1;
            }
        }
    }

    // Swap buffers
    free(sim->temperature_map);
    sim->temperature_map = new_temperature_map;
    sim->t += 1;
}

void dispose(Simulation* sim) {
    // free(sim->height_map);
    // free(sim->vegetation_map);
    free(sim->temperature_map);
    free(sim);
}