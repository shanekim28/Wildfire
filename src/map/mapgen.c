#define FNL_IMPL
#include "map/mapgen.h"
#include "FastNoiseLite.h"
#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

static void generate_height_map(float** height_map, int width, int height, int seed);
static void generate_water_map(float** water_map, float** height_map, int width, int height, int seed);
void generate_vegetation_map(float** vegetation_map, int width, int height, int seed);
void generate_road_map(float** road_map, int width, int height, int seed);
void generate_structure_map(float** structure_map, int width, int height, int seed);

void init_map(Map* map, int width, int height, int seed) {
    map->width = width;
    map->height = height;
    map->seed = seed;

    map->height_map = malloc(width * height * sizeof(float));
    map->water_map = malloc(width * height * sizeof(float));
    map->vegetation_map = malloc(width * height * sizeof(float));
    map->road_map = malloc(width * height * sizeof(float));
    map->structure_map = malloc(width * height * sizeof(float));
}

void generate_map(Map* map) {
    int width = map->width;
    int height = map->height;
    memset(map->height_map, 0, width * height * sizeof(float));
    memset(map->water_map, 0, width * height * sizeof(float));
    generate_height_map(&(map->height_map), width, height, map->seed);
    generate_water_map(&(map->water_map), &(map->height_map), width, height, map->seed);
}

void free_map(Map* map) {
    free(map->height_map);
    free(map->water_map);
    free(map->vegetation_map);
    free(map->road_map);
    free(map->structure_map);
}

/// BEGIN HELPER METHODS
static void generate_height_map(float** height_map, int width, int height, int seed) {
    // Generate height map
    fnl_state noise = fnlCreateState();
    noise.noise_type = FNL_NOISE_OPENSIMPLEX2;
    noise.seed = seed;
    noise.frequency = 0.0075f;
    noise.fractal_type = FNL_FRACTAL_FBM;
    noise.octaves = 7;
    noise.lacunarity = 1.79f;
    noise.gain = 0.36f;

    int index = 0;
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            (*height_map)[index++] = (fnlGetNoise2D(&noise, x, y) + 1) / 2.0f;
        }
    }
}

static void generate_water_map(float** water_map, float** height_map, int width, int height, int seed) {
    // Configurations
    float w_elevation = 0.6f;
    float w_curve = 0.4f;
    int r_step = 5;
    int divisions = 10;
    float alph_dev = 2 * M_PI / 10.0f;

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            if ((*height_map)[y * width + x] < 0.1f) {
                (*water_map)[y * width + x] = 1;
            }
        }
    }

    // Generate control pairs (p_src, p_dst)
    printf("# GENERATE RIVERS\n");
    srand(seed);
    int px_src = (int)(rand() / (1.0f + RAND_MAX / (width)));
    int py_src = (int)(rand() / (1.0f + RAND_MAX / (height)));

    int px_dst = (int)(rand() / (1.0f + RAND_MAX / (width)));
    int py_dst = (int)(rand() / (1.0f + RAND_MAX / (height)));

    int px_cur = px_src;
    int py_cur = py_src;

    (*water_map)[py_cur * width + px_cur] = 1;

    printf("%d %d | %d %d\n", px_src, py_src, px_dst, py_dst);
    while (abs(px_cur - px_dst) > 10 || abs(py_cur - py_dst) > 10) {
        float alph = atan2(py_dst - py_cur, px_dst - px_cur);
        float res = -INFINITY;
        int resx = 0;
        int resy = 0;

        // Create n new candidate points p_can on circle with range r_step at interval alph - alph_dev
        for (int i = -divisions; i <= divisions; i += 2) {
            int px_can = px_cur + r_step * cos(alph + i * alph_dev / (float)divisions);
            int py_can = py_cur + r_step * sin(alph + i * alph_dev / (float)divisions);

            float z_cur = (*height_map)[py_cur * width + px_cur];
            if (!(px_can >= 0 && px_can < width && py_can >= 0 && py_can < height)) {
                continue;
            }
            float z_can = (*height_map)[py_can * width + px_can];
            // s_elevation = (p_cur * elevation - p_can * elevation) / magnitude(p_can - p_cur)
            float s_elevation = (z_cur - z_can) / sqrt(pow(px_can - px_cur, 2) + pow(py_can - py_cur, 2));

            float a_x = (px_dst - px_cur) / sqrt(pow(px_dst - px_cur, 2) + pow(py_dst - py_cur, 2));
            float a_y = (py_dst - py_cur) / sqrt(pow(py_dst - py_cur, 2) + pow(py_dst - py_cur, 2));
            float b_x = (px_can - px_cur) / sqrt(pow(px_can - px_cur, 2) + pow(py_can - py_cur, 2));
            float b_y = (py_can - py_cur) / sqrt(pow(py_can - py_cur, 2) + pow(py_can - py_cur, 2));
            // s_curve = 1 - arccos(dot((p_dst - p_cur) / magnitude(p_dst - p_cur), (p_can - p_cur) / magnitude(p_can - p_cur))) / alph_dev
            float s_curve = 1 - acos(a_x * b_x + a_y * b_y) / alph_dev;

            // Find highest candidate score thru weighted sum s_can = w_elevation * s_elevation + w_curve * s_curve
            float s_can = w_elevation * s_elevation + w_curve * s_curve;

            if (s_can > res) {
                resx = px_can;
                resy = py_can;
                res = s_can;
            }
        }

        // plot line in water map
        int x0 = px_cur;
        int y0 = py_cur;
        int x1 = resx;
        int y1 = resy;

        int dx = x1 - x0;
        int dy = y1 - y0;

        int dLong = abs(dx);
        int dShort = abs(dy);

        int offsetLong = dx > 0 ? 1 : -1;
        int offsetShort = dy > 0 ? width : -width;

        if(dLong < dShort) {
            dShort = dShort ^ dLong;
            dLong = dShort ^ dLong;
            dShort = dShort ^ dLong;

            offsetShort = offsetShort ^ offsetLong;
            offsetLong = offsetShort ^ offsetLong;
            offsetShort = offsetShort ^ offsetLong;
        }

        int error = 2 * dShort - dLong;
        int index = y0 * width + x0;
        const int offset[] = {offsetLong, offsetLong + offsetShort};
        const int abs_d[]  = {2 * dShort, 2 * (dShort - dLong)};
        for(int i = 0; i <= dLong; ++i) {
            (*water_map)[index] = 255;  // or a call to your painting method
            const int errorIsTooBig = error >= 0;
            index += offset[errorIsTooBig];
            error += abs_d[errorIsTooBig];
        }

        px_cur = resx;
        py_cur = resy;
        printf("%d %d\n", px_cur, py_cur);
    }
}

/// END HELPER METHODS
