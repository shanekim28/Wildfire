#ifndef WILDFIRE_MAPGEN_H
#define WILDFIRE_MAPGEN_H
typedef struct {
    int width;
    int height;
    int seed;
    float* height_map;
    float* water_map;
    float* vegetation_map;
    float* road_map;
    float* structure_map;
} Map;

void init_map(Map* map, int width, int height, int seed);
void generate_map(Map* map);
void free_map(Map* map);
#endif
