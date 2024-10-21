#ifndef FIRESIM_H
#define FIRESIM_H

#include "FastNoiseLite.h"

#define BASE_IGNITION_PROBABILITY 0.58
#define WIND_COEFF 0.3
#define C1 0.045
#define C2 0.131
#define VEGETATION_COEFF 0.3
#define SLOPE_COEFF 0.3

typedef struct Vector2 {
    float x;
    float y;
} Vector2;

typedef struct Simulation {
    int width;
    int height;
    float* height_map;
    float* vegetation_type_map;
    float* vegetation_density_map;
    Vector2* wind_map;
    float* temperature_map;
    fnl_state noise;
    float t;
} Simulation;

void init_fire_sim(Simulation* sim, int width, int height);
void advance_fire_sim(Simulation* sim);
void advance_wind_sim(Simulation* sim);
void dispose(Simulation* sim);
#endif