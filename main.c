
#include "libs/include/SDL.h"
#include "libs/include/SDL_render.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600

#define AMOUNT 100


struct vector_s {
    float x;
    float y;
};

typedef struct vector_s vector;

float screen_max_fw;
float screen_max_fh;

float G = 6.67e-11f;
float pmass = 100;

vector particles[AMOUNT];
vector accelerations[AMOUNT];

float mapf(float x, float in_min, float in_max, float out_min, float out_max) {
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}


void vector_add(vector *v1, const vector *v2){
    v1->x += v2->x;
    v1->y += v2->y;
}
void vector_multiply_f(vector *v, float f){
    v->x *= f;
    v->y *= f;
}
void vector_divide_f(vector *v, float f){
    v->x /= f;
    v->y /= f;
}


SDL_Window *window;
SDL_Renderer *renderer;

void start(){
    if(SCREEN_WIDTH > SCREEN_HEIGHT){
        screen_max_fw = 1;
        screen_max_fh = (float) SCREEN_HEIGHT / SCREEN_WIDTH;
    }else{
        screen_max_fw = (float) SCREEN_WIDTH / SCREEN_HEIGHT;
        screen_max_fh = 1;
    }

    //printf("max w: %f, max h: %f", screen_max_fw, screen_max_fh);
    for(int i = 0; i < AMOUNT; i++){
        particles[i].x = (float) rand() / (float) RAND_MAX;
        particles[i].y = (float) rand() / (float) RAND_MAX;
    }
}

float distancePow2(const vector* p1, const vector* p2){
    return pow(p1->x - p2->x, 2) + pow(p1->y - p2->y, 2);
}
float distance(const vector* p1, const vector* p2){
    return sqrt(distancePow2(p1, p2));
}

void calculate_forces(const vector* particle, vector* pforce){
    vector allforce = {0.0f,0.0f};

    for(int i = 0; i < AMOUNT; i++){
        vector *a = &particles[i];
        float dist = distance(particle, a);
        if(dist < 0.0000001)
            continue;
        float force = 10 / dist * dist;
        float dx = a->x - particle->x;
        float dy = a->y - particle->y;

        vector forcev;
        forcev.x = force * dx / dist;
        forcev.y = force * dy / dist; 
        vector_add(&allforce, &forcev);
    }

    pforce->x = allforce.x; 
    pforce->y = allforce.y; 
}

float mult = 0.0001;
void apply_next_position(vector* particle, const vector* force){
    particle->x = particle->x + force->x / pmass * mult;
    particle->y = particle->y + force->y /pmass * mult;
}

void loop(){
    vector forces[AMOUNT];
    for(int i = 0; i < AMOUNT; i++){
        calculate_forces(&particles[i], &forces[i]);
    }
    //    printf("x: %f, y: %f\n", particles[0].x, particles[0].y);
    for(int i = 0; i < AMOUNT; i++){
        accelerations[i].x += forces[i].x / pmass; 
        accelerations[i].y += forces[i].y / pmass; 


        vector *particle = &particles[i];

        vector *acceleration = &accelerations[i];

        particle->x = particle->x + acceleration->x * mult;
        particle->y = particle->y + acceleration->y * mult;
        SDL_RenderDrawPoint(renderer, (int) (particles[i].x * SCREEN_WIDTH),(int) (particles[i].y * SCREEN_HEIGHT));
    }

}




int main(){
    window = SDL_CreateWindow("Test", 200, 200, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_OPENGL);
    if (window == NULL) {
        printf("Error window creation\n");
        return 3;
    }
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC);

    start();

    while(1){
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
}








