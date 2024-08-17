#define FNL_IMPL
#include "FastNoiseLite.h"
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