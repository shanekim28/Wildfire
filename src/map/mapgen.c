#define FNL_IMPL
#include "map/mapgen.h"
#include "FastNoiseLite.h"
#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "vector.h"
#include "util.h"

static void generate_height_map(float** height_map, int width, int height, int seed);
static void generate_water_map(float** water_map, float** height_map, int width, int height, int seed);
void generate_vegetation_map(float** vegetation_map, int width, int height, int seed);
void generate_road_map(float** road_map, int width, int height, int seed);
void generate_structure_map(float** structure_map, int width, int height, int seed);

typedef struct DLAPoint DLAPoint;
struct DLAPoint {
    Vector2Int pos;
    DLAPoint* parent;
    DLAPoint* child;
    int weight;
};

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
    //generate_water_map(&(map->water_map), &(map->height_map), width, height, map->seed);
}

void free_map(Map* map) {
    free(map->height_map);
    free(map->water_map);
    free(map->vegetation_map);
    free(map->road_map);
    free(map->structure_map);
}


/// BEGIN HELPER METHODS
static void print_map(DLAPoint* points[], int width, int height) {
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            DLAPoint* curP = points[y * width + x];
            if (curP == NULL) {
                printf(". ");
                continue;
            }

            if (curP->parent != NULL) {
                int dirX = curP->parent->pos.x - curP->pos.x;
                int dirY = curP->parent->pos.y - curP->pos.y;

                if (dirX < 0) {
                    printf("← ");
                } else if (dirX > 0){
                    printf("→ ");
                } else if (dirY > 0) {
                    printf("↓ ");
                } else if (dirY < 0) {
                    printf("↑ ");
                }
            } else {
                printf("# ");
            }
        }
        printf("\n");
    }
}

static void upscale(DLAPoint** points[], int width, int height, int scaleFactor) {
    printf("# Upscaling from %dx%d to %dx%d\n", width, height, width * scaleFactor, height * scaleFactor);
    for (int i = 1; i < scaleFactor; i *= 2){
        int newWidth = width * 2;
        int newHeight = height * 2;
        DLAPoint** newPoints = NULL;
        newPoints = calloc(newWidth * newHeight, sizeof(DLAPoint*));

        // Upscale crisp
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                DLAPoint* curPoint = (*points)[y * width + x];
                if (curPoint == NULL) {
                    continue;
                }

                curPoint->pos.x *= 2;
                curPoint->pos.y *= 2;
                newPoints[curPoint->pos.y * newWidth + curPoint->pos.x] = curPoint;
            }
        }

        free(*points);
        *points = newPoints;

        width = newWidth;
        height = newHeight;
    }
}

static void create_intermediate_points(DLAPoint** points[], int width, int height) {
    DLAPoint** newPoints = NULL;
    newPoints = calloc(width * height, sizeof(DLAPoint*));
    
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            DLAPoint* curP = (*points)[y * width + x];
            if (curP == NULL) {
                continue;
            }

            DLAPoint* between = NULL;
            if (curP->parent != NULL) {
                between = malloc(sizeof(DLAPoint));
                between->weight = 0;
                between->pos.x = (curP->pos.x + curP->parent->pos.x) / 2;
                between->pos.y = (curP->pos.y + curP->parent->pos.y) / 2;
                between->parent = curP->parent;
                curP->parent->child = between;
                between->child = curP;
                curP->parent = between;

                newPoints[between->pos.y * width + between->pos.x] = between;
            }

            newPoints[curP->pos.y * width + curP->pos.x] = curP;
        }
    }

    free(*points);
    *points = newPoints;
}

static void DLA(DLAPoint** points, int width, int height, int numPoints, bool startNew) {
    printf("# Running DLA using %d points with size %dx%d\n", numPoints, width, height);

    // Populate
    int i = 0;
    while (i < numPoints) {
        // Points from outer circle
        int px = rand_range(0, width - 1);
        int py = rand_range(0, height - 1);

        if (points[py * width + px] != NULL) {
            continue;
        }

        Vector2Int pos = { .x = px, .y = py };
        DLAPoint* curPoint = malloc(sizeof(DLAPoint));
        curPoint->weight = 0;
        curPoint->pos = pos;
        curPoint->parent = NULL;
        curPoint->child = NULL;
        points[py * width + px] = curPoint;

        if (i == 0 && startNew) {
            i++;
            continue;
        }

        while (1) {
            bool touching = false;
            // Check if collided
            for (int j = 0; j < 4 && !touching; j++) {
                int dx = wf_dirx[j];
                int dy = wf_diry[j];
                if (!is_in_bounds(curPoint->pos.x + dx, curPoint->pos.y + dy, width, height)) continue;

                DLAPoint* neighbor = points[(curPoint->pos.y + dy) * width + (curPoint->pos.x + dx)];
                if (neighbor != NULL) {
                    curPoint->parent = neighbor;
                    neighbor->child = curPoint;
                    touching = true;
                }
            }
            if (touching) break;

            points[curPoint->pos.y * width + curPoint->pos.x] = NULL;

            int rand_index = (int)(rand_range(0, 3));
            int dirX = wf_dirx[rand_index];
            int dirY = wf_diry[rand_index];

            curPoint->pos.x = clamp(curPoint->pos.x + dirX, 0, width - 1);
            curPoint->pos.y = clamp(curPoint->pos.y + dirY, 0, height - 1);

            points[curPoint->pos.y * width + curPoint->pos.x] = curPoint;
        }

        i++;
    }
}

static void blurry_upscale(float** height_map, int width, int height, int scaleFactor) {
    // Bilinear interpolation
    int i = 1;
    while (i < scaleFactor) {
        int newWidth = width * 2;
        int newHeight = height * 2;
        float* upscaled = malloc(newWidth * newHeight * sizeof(float));
        memset(upscaled, 0, newWidth * newHeight * sizeof(float));

        for (int y = 0; y < newHeight; y++) {
            for (int x = 0; x < newWidth; x++) {
                float gx = (float)(x) / 2;
                float gy = (float)(y) / 2;

                int x0 = (int)gx;
                int y0 = (int)gy;
                int x1 = x0 + 1 < width ? x0 + 1 : x0;
                int y1 = y0 + 1 < height ? y0 + 1 : y0;

                float tx = gx - x0;
                float ty = gy - y0;

                float vx0 = (*height_map)[y0 * width + x0];
                float vx1 = (*height_map)[y0 * width + x1];
                float vy0 = (*height_map)[y1 * width + x0];
                float vy1 = (*height_map)[y1 * width + x1];
                float topInterp = (1 - tx) * vx0 + tx * vx1;
                float bottomInterp = (1 - tx) * vy0 + tx * vy1;

                float val = (1 - ty) * topInterp + ty * bottomInterp;
                upscaled[y * newWidth + x] = val;
            }
        }

        // Convolve
        float* convoluted = malloc(newWidth * newHeight * sizeof(float));
        memset(convoluted, 0, newWidth * newHeight * sizeof(float));
        float kernel[4][4] = {
            {0.125, 0.125, 0.125, 0.125},
            {0.125, 0.125, 0.125, 0.125},
            {0.125, 0.125, 0.125, 0.125},
            {0.125, 0.125, 0.125, 0.125},
        };
        for (int y = 0; y < newHeight - 4; y++) {
            for (int x = 0; x < newWidth - 4; x++) {
                float sum = 0.f;
                for (int ky = 0; ky < 4; ky++) {
                    for (int kx = 0; kx < 4; kx++) {
                        sum += upscaled[(y + ky) * newWidth + (x + kx)] * kernel[ky][kx];
                    }
                }

                convoluted[y * newWidth + x] = sum;
            }
        }

        free(upscaled);
        free(*height_map);
        *height_map = convoluted;

        width *= 2;
        height *= 2;
        i *= 2;
    }
}

static void points_to_heightmap(float** height_map, DLAPoint* points[], int width, int height) {
    float* newMap = calloc(width * height, sizeof(float));
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            if (points[y * width + x] != NULL)
                newMap[y * width + x] = 1;
        }
    }

    *height_map = newMap;
}

static void sum_heightmaps(float* a, float* b, int width, int height) {
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            a[y * width + x] += b[y * width + x];
        }
    }
}

static void generate_height_map(float** height_map, int width, int height, int seed) {
    // Perform DLA with a gaussian blur
    srand(seed);
    rand();

    int initialWidth = 8;
    int initialHeight = 8;
    int numPoints = 30;

    DLAPoint** points = malloc(initialWidth * initialHeight * sizeof(DLAPoint*));
    memset(points, 0, initialWidth * initialHeight * sizeof(DLAPoint*));

    DLA(points, initialWidth, initialHeight, numPoints, true);
    print_map(points, initialWidth, initialHeight);

    int scaleFactor = 16;
    int scaleStep = 2;
    int jitter = 2;
    float* base_image;
    float* newDetail;

    // Create base image
    points_to_heightmap(&base_image, points, initialWidth, initialHeight);

    float max_weight = 1;
    for (int i = 1; i < scaleFactor; i *= scaleStep) {
        // blur
        blurry_upscale(&base_image, initialWidth, initialHeight, scaleStep);

        // Draw detail
        upscale(&points, initialWidth, initialHeight, scaleStep);

        initialWidth *= scaleStep;
        initialHeight *= scaleStep;
        print_map(points, initialWidth, initialHeight);
        float* tmp = calloc(initialWidth * initialHeight, sizeof(float));
        // Assign weights
        for (int y = 0; y < initialHeight; y++) {
            for (int x = 0; x < initialWidth; x++) {
                DLAPoint* cur = points[y * initialWidth + x];
                if (cur == NULL) continue;

                // Find leaf nodes
                if (cur->child != NULL) continue;

                float weight = 0;
                DLAPoint* it = cur;
                while (it->parent) {
                    weight++;

                    it->parent->weight = weight > (it->parent->weight) ? weight : it->parent->weight;

                    if (weight > max_weight) {
                        max_weight = weight;
                    }
                    
                    it = it->parent;
                }
            }
        }

        // draw lines between
        for (int y = 0; y < initialHeight; y++) {
            for (int x = 0; x < initialWidth; x++) {
                DLAPoint* cur = points[y * initialWidth + x];
                if (cur == NULL) continue;

                if (cur->parent == NULL) continue;
                /*
                int childPos = it->pos.y * initialWidth + it->pos.x;
                int betweenPos = betweeny * initialWidth + betweenx;
                int parentPos = it->parent->pos.y * initialWidth + it->parent->pos.x;
                */
                int betweenx = (cur->pos.x + cur->parent->pos.x) / 2 + rand_range(-jitter, jitter);
                int betweeny = (cur->pos.y + cur->parent->pos.y) / 2 + rand_range(-jitter, jitter);
                betweenx = clamp(betweenx, 0, initialWidth - 1);
                betweeny = clamp(betweeny, 0, initialHeight - 1);

                draw_line(tmp, cur->pos.x, cur->pos.y, betweenx, betweeny, initialWidth, cur->parent->weight, WF_DRAW_MODE_NORMAL);
                draw_line(tmp, betweenx, betweeny, cur->parent->pos.x, cur->parent->pos.y, initialWidth, cur->parent->weight, WF_DRAW_MODE_NORMAL);
            }
        }

        sum_heightmaps(base_image, tmp, initialWidth, initialHeight);

        // Generate new detail
        for (int j = 1; j < scaleStep; j *= 2) {
            create_intermediate_points(&points, initialWidth, initialHeight);
        }
        DLA(points, initialWidth, initialHeight, (initialWidth) * (initialHeight) * ((float)9 / 25), false);

        numPoints *= scaleStep;
    }

    blurry_upscale(&base_image, initialWidth, initialHeight, 2);
    initialWidth *= 2;
    initialHeight *= 2;
    for (int y = 0; y < initialHeight; y++) {
        for (int x = 0; x < initialWidth; x++) {
            (*height_map)[y * width + x] = 1 - (1 / ((1 + base_image[y * initialWidth + x] / max_weight)));
            //(*height_map)[y * width + x] = 1 - (1 / ((1 + base_image[y * initialWidth + x])));
            //(*height_map)[y * width + x] = base_image[y * initialWidth + x] / max_weight;
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
        draw_line(*water_map, px_cur, py_cur, resx, resy, width, 1, WF_DRAW_MODE_NORMAL);

        px_cur = resx;
        py_cur = resy;
        //printf("%d %d\n", px_cur, py_cur);
    }
}

/// END HELPER METHODS
