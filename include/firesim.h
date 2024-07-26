#ifndef FIRESIM_H
#define FIRESIM_H

#define BASE_IGNITION_PROBABILITY 0.3
#define WIND_COEFF 0.3
#define VEGETATION_COEFF 0.3
#define SLOPE_COEFF 0.3

#define WF_LEFT 1
#define WF_UP 2
#define WF_RIGHT 3
#define WF_DOWN 4

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
} Simulation;

void init_fire_sim(Simulation* sim, int width, int height);
void advance_fire_sim(Simulation* sim);
void dispose(Simulation* sim);
#endif