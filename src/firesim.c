#define JC_VORONOI_IMPLEMENTATION
#include "FastNoiseLite.h"
#include "firesim.h"
#include <stdlib.h>
#include <libc.h>
#include "jc_voronoi.h"
#include "util.h"

void generate_heightmap(float* height_map, int width, int height, int seed) {
    // Delaunay triangulation
    int num_points = 100;

    int i;
    jcv_rect bounding_box = { { 0.0f, 0.0f }, { width, height } };
    jcv_diagram diagram;
    jcv_point points[num_points];
    const jcv_site* sites;
    jcv_graphedge* graph_edge;

    memset(&diagram, 0, sizeof(jcv_diagram));

    srand(seed);
    for (i = 0; i < num_points; i++) {
        points[i].x = (int)(rand()/(1.0f + RAND_MAX / (width + 1)));
        points[i].y = (int)(rand()/(1.0f + RAND_MAX / (height + 1)));

    }

    printf("# Seed sites\n");
    for (i = 0; i < num_points; i++) {
        printf("%f %f\n", points[i].x, points[i].y);
    }

    jcv_diagram_generate(num_points, (const jcv_point*) points, &bounding_box, 0, &diagram);

    printf("# Edges\n");
    sites = jcv_diagram_get_sites(&diagram);
    for (i = 0; i < diagram.numsites; i++) {
        graph_edge = sites[i].edges;

        while (graph_edge) {
              // This approach will potentially print shared edges twice
              printf("%f %f\n", graph_edge->pos[0].x, graph_edge->pos[0].y);
              printf("%f %f\n", graph_edge->pos[1].x, graph_edge->pos[1].y);
              graph_edge = graph_edge->next;
        }
    }

    // Lloyd relaxation
    int j;
    for (j = 0; j < 50; j++) {
        for (i = 0; i < diagram.numsites; i++) {
            const jcv_site* site = &sites[i];
            jcv_point sum = site->p;
            int count = 1;

            const jcv_graphedge* edge = site->edges;
            while (edge) {
                sum.x += edge->pos[0].x;
                sum.y += edge->pos[0].y;
                count++;
                edge = edge->next;
            }

            points[site->index].x = sum.x / count;
            points[site->index].y = sum.y / count;
        }
    }
    
    for (i = 0; i < diagram.numsites; i++) {
        height_map[(int)(points[i].x + points[i].y * width)] = 1;
    }

    jcv_diagram_free(&diagram);

}

// Generate a fire map with specified width and height
void init_fire_sim(Simulation* sim, int width, int height) {
    // TODO Initialize simulation
    // Generate topography
    //sim->height_map = malloc(width * height * sizeof(float));
    //generate_heightmap(sim->height_map, width, height, 1337);

    Map* map = malloc(sizeof(Map));
    memset(map, 0, sizeof(Map));
    init_map(map, width, height, 1337);
    generate_map(map);
    sim->map = map;

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

int dx[] = {-1, 1, 0, 0};
int dy[] = {0, 0, -1, 1};
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
        for (int i = 0; i < 4; i++) {
            int neighborRow = dx[i];
            int neighborCol = dy[i];
        //for (int neighborRow = -1; neighborRow <= 1; neighborRow++) {
            //for (int neighborCol = -1; neighborCol <= 1; neighborCol++) {
                // Ignore current cell
                if (neighborRow == 0 && neighborRow == neighborCol)
                    continue;

                // Bounds check
                if (!is_in_bounds(col + neighborCol, row + neighborRow,
                                 sim->width, sim->height))
                    continue;

                // Ignore burned/burning cells
                int neighborIndex = row_col_to_index(
                    row + neighborRow, col + neighborCol, sim->width);
                if (sim->temperature_map[neighborIndex] >= 0 ||
                    new_temperature_map[neighborIndex] >= 0)
                    continue;

                // Ignore water cells
                if (sim->map->water_map[neighborIndex] > 0) {
                    continue;
                }
                
                // Ignite cell
                float rand = (random() % 100) / 100.0f;
                if (rand < ignition_probability)
                    new_temperature_map[neighborIndex] = 1;
            //}
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
