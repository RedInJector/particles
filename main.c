
#include <CL/cl_platform.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_render.h>

#include "opencl_physics.c"


#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600

#define TILES_V 5
#define TILES_H 5

struct vector_f_s {
    float x;
    float y;
};

typedef struct vector_f_s vectorf;

struct vector_i_s {
    int x;
    int y;
};

typedef struct vector_i_s vectori;

float mapf(float x, float in_min, float in_max, float out_min, float out_max) {
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

void vector_add(vectorf *v1, const vectorf *v2) {
    v1->x += v2->x;
    v1->y += v2->y;
}
void vector_multiply_f(vectorf *v, float f) {
    v->x *= f;
    v->y *= f;
}
void vector_divide_f(vectorf *v, float f) {
    v->x /= f;
    v->y /= f;
}
float distancePow2(const vectorf *p1, const vectorf *p2) {
    return pow(p1->x - p2->x, 2) + pow(p1->y - p2->y, 2);
}
float distance(const vectorf *p1, const vectorf *p2) {
    return sqrt(distancePow2(p1, p2));
}

float G = 6.67e-11f;
float pmass = 100;

bool draw_grid = false;
float tile_size_H = 1.0 / TILES_H;
float tile_size_V = 1.0 / TILES_V;

int particle_amount = 10;
float mult = 0.00001;

vectorf *particles;
vectorf *accelerations;
float tile_masses[TILES_V * TILES_H];

vectorf tiles[TILES_V * TILES_H];
vectorf tile_forces[TILES_V * TILES_H];

float screen_max_fw;
float screen_max_fh;
SDL_Window *window;
SDL_Renderer *renderer;
SDL_Surface *wsurface;


vectori findTile(vectorf *particle){
    vectori coordinates;
    coordinates.x = fmin(fmax((int)(particle->x / tile_size_H), 0), TILES_H - 1);
    coordinates.y = fmin(fmax((int)(particle->y / tile_size_V), 0), TILES_V - 1);
    return coordinates;
}

vectorf *getTile(int row, int col){
    return &tiles[row * TILES_H + col];
}
float *getMass(int row, int col){
    return &tile_masses[row * TILES_H + col];
}
vectorf *getTileForce(int row, int col){
    return &tile_forces[row * TILES_H + col];
}

void start() {
    if (SCREEN_WIDTH > SCREEN_HEIGHT) {
        screen_max_fw = 1;
        screen_max_fh = (float)SCREEN_HEIGHT / SCREEN_WIDTH;
    } else {
        screen_max_fw = (float)SCREEN_WIDTH / SCREEN_HEIGHT;
        screen_max_fh = 1;
    }

    for (int i = 0; i < TILES_V; i++) {
        for (int j = 0; j < TILES_H; j++) {
            getTile(i, j)->x = tile_size_H * j;
            getTile(i, j)->y = tile_size_H * i;
        }
    }
    for (int i = 0; i < particle_amount; i++) {
        particles[i].x = (float)rand() / (float)RAND_MAX;
        particles[i].y = (float)rand() / (float)RAND_MAX;
    }
}


int parse_args(int argc, char **argv) {
    printf("arguments c: %d\n", argc);

    if (argc == 1)
        return 0;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-p") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Missing value for -p\n");
                return 1;
            }

            char *endptr;
            long num = strtol(argv[i + 1], &endptr, 10);
            if (*endptr != '\0') {
                fprintf(stderr, "Invalid number: %s\n", argv[i + 1]);
                return 1;
            }

            if (num < INT_MIN || num > INT_MAX) {
                fprintf(stderr, "Invalid int: %s\n", argv[i + 1]);
                return 1;
            }

            particle_amount = (int) num;
            i++;
        } else if (strcmp(argv[i], "-g") == 0) {
            draw_grid = true;
        } else {
            fprintf(stderr, "Unknown option: %s\n", argv[i]);
            return 1;
        }
    }

    return 0;
}


void loop() {
    vectorf forces[particle_amount];

    for (int i = 0; i < TILES_V; i++) {
        for (int j = 0; j < TILES_H; j++) {
            *getMass(i,j) = 0;
        }
    }
    for (int i = 0; i < particle_amount; i++) {
        vectorf *p = &particles[i];
        vectori tile = findTile(p); 
        *getMass(tile.y, tile.x) += pmass;
    }
    //calculate_tile_forces();
    printf("here1\n");
    PX_calculate_physics((cl_float2*)tiles, tile_masses, (cl_float*)tile_forces, TILES_H, TILES_V);    
    printf("here2\n");
    for (int i = 0; i < particle_amount; i++) {
        vectorf *particle = &particles[i];
        vectori tile = findTile(particle);
        accelerations[i].x += getTileForce(tile.y, tile.x)->x / pmass;
        accelerations[i].y += getTileForce(tile.y, tile.x)->y / pmass;

        vectorf *acceleration = &accelerations[i];

        particle->x = particle->x + acceleration->x * mult;
        particle->y = particle->y + acceleration->y * mult;
        if(particle->x > 1){
            particle->x = 1;
        }
        if(particle->x < 0){
            particle->x = 0;
        }
        if(particle->y > 1){
            particle->y = 1;
        }
        if(particle->y < 0){
            particle->y = 0;
        }
        SDL_RenderDrawPoint(renderer, (int)(particles[i].x * SCREEN_WIDTH),
                (int)(particles[i].y * SCREEN_HEIGHT));
    }
    if(draw_grid) 
        for (int i = 0; i < TILES_V; i++) {
            for (int j = 0; j < TILES_H; j++) {
                vectorf *t = getTile(i, j);
                vectori vi;
                vi.x = (int)(t->x * SCREEN_WIDTH);
                vi.y = (int)(t->y * SCREEN_HEIGHT);
                SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);
                SDL_RenderDrawLine(renderer, vi.x, vi.y,
                        vi.x + tile_size_H * SCREEN_WIDTH, vi.y);
                SDL_RenderDrawLine(renderer, vi.x, vi.y, vi.x,
                        vi.y + tile_size_V * SCREEN_HEIGHT);
            }
        }

}




int main(int argc, char **argv) {
    if(PX_setupCL() != 0)
        return 1;

    PX_allocate_gpu_buffers(TILES_H, TILES_V);
    PX_set_gpu_kernel_args(TILES_H, TILES_V);

    particle_amount = 100;
    draw_grid = false;

    int isparsed = parse_args(argc, argv);
    if(isparsed != 0)
        return isparsed;

    particles = malloc(sizeof(vectorf) * particle_amount);
    accelerations = malloc(sizeof(vectorf) * particle_amount);

    window = SDL_CreateWindow("Test", 200, 200, SCREEN_WIDTH, SCREEN_HEIGHT,
            SDL_WINDOW_OPENGL);
    if (window == NULL) {
        printf("Error window creation\n");
        return 3;
    }
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC);
    wsurface = SDL_GetWindowSurface(window);

    start();
    while (1) {
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        loop();
        SDL_RenderPresent(renderer);
        SDL_Event e;
        if (SDL_PollEvent(&e) > 0) {
            if (e.type == SDL_QUIT) {
                break;
            }
        }
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    PX_clearCL();
}
