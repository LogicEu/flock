#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <glee.h>
#include <photon.h>

#define SCR_WIDTH 800
#define SCR_HEIGHT 600

static vec2 resolution;
static float scale = 1.0;

#define DIF_SCALE 1.0
#define DIF_X 0.01 * DIF_SCALE
#define DIF_Y 0.02 * DIF_SCALE
#define OFF_ROT -90.0

#define FLOCK_SPEED 0.01
#define FLOCK_SEPARATION 0.005
#define FLOCK_NEIGHBOUR 1.0

static unsigned int flockId, shader_back, shader_triangle;

vec2 *posptr, *velptr;
vec4 *colptr;
Tri2D *flockptr;

static unsigned int indices[] = {
    0,  1,  2
};

vec4 vec4_rand()
{
    vec4 v = {randf_norm(), randf_norm(), randf_norm(), 1.0};
    return v;
}

vec2 world_to_screen(vec2 p)
{
    vec2 v;
    v.x = (p.x + 0.5) * resolution.x * scale;
    v.y = (p.y + 0.5) * resolution.y * scale;
    return v;
}

Tri2D spawn_tri(vec2 pos, float rot)
{
    Tri2D t;
    t.a = vec2_add(pos, vec2_new(DIF_X, -DIF_Y));
    t.b = vec2_add(pos, vec2_new(0.0, DIF_Y));
    t.c = vec2_add(pos, vec2_new(-DIF_X, -DIF_Y));
    t.a = vec2_rotate_around(t.a, pos, rot + deg_to_rad(OFF_ROT));
    t.b = vec2_rotate_around(t.b, pos, rot + deg_to_rad(OFF_ROT));
    t.c = vec2_rotate_around(t.c, pos, rot + deg_to_rad(OFF_ROT));
    return t;
}

Tri2D* spawn_flock(unsigned int count)
{
    Tri2D* flock = malloc(sizeof(Tri2D) * count);
    for (int i = 0; i < count; i++) {
        posptr[i] = vec2_mult(vec2_rand(), 0.5);
        velptr[i] = vec2_mult(vec2_rand(), 5.0);
        flock[i] = spawn_tri(posptr[i], vec2_to_rad(velptr[i]));
        colptr[i] = vec4_rand();
    }
    return flock;
}

void tri_bind(Tri2D* tri)
{
    glee_buffer_create_indexed(flockId, tri, sizeof(Tri2D), &indices[0], sizeof(indices));
    glee_buffer_attribute_set(0, 2, 0, 0);
}

void flock_update(unsigned int count, float deltaTime)
{
    deltaTime *= 50.0;

    glUseProgram(shader_triangle);
    glBindVertexArray(flockId);
    glee_shader_uniform_set(shader_triangle, 1, "u_scale", &scale);

    for (unsigned int i = 0; i < count; i++) {
        
        vec2 separation = {0.0, 0.0};
        vec2 alignment = {0.0, 0.0};
        vec2 cohesion = {0.0, 0.0};

        unsigned int shortNeighbours, neighbours;
        shortNeighbours = neighbours = 0;

        for (unsigned int j = 0; j < count; j++) {
            
            if (i == j) continue;
            
            float sqdist = absf(vec2_sqdist(posptr[i], posptr[j]));
            vec2 dif = _vec2_sub(posptr[i], posptr[j]);
            
            if (sqdist < FLOCK_SEPARATION) {
                //Separation: short space repulsion
                separation = vec2_add(separation, vec2_mult(vec2_norm(dif), FLOCK_SPEED / sqdist));
                shortNeighbours++;
            }

            if (sqdist < FLOCK_NEIGHBOUR) {
                //Alignment: average of neighbours direction
                alignment = vec2_add(alignment, vec2_mult(vec2_norm(velptr[j]), sqdist));
                //Cohesion: average position of neighbours
                cohesion = vec2_add(cohesion, vec2_mult(vec2_norm(dif), -sqdist));
                neighbours++;
            }
        }
        
        if (shortNeighbours) separation = vec2_div(separation, shortNeighbours);
        if (neighbours) {
            alignment = vec2_div(alignment, neighbours);
            cohesion = vec2_div(cohesion, neighbours);
        }

        vec2 force = vec2_add(vec2_add(separation, alignment), cohesion);
        velptr[i] = vec2_add(velptr[i], vec2_mult(force, deltaTime));

        float px = posptr[i].x * scale - 0.45;
        float py = posptr[i].y * scale - 0.45;
        if (posptr[i].x * scale > 0.45 && px != 0) velptr[i].x -= deltaTime / px;
        if (posptr[i].y * scale > 0.45 && py != 0) velptr[i].y -= deltaTime / py;
        if (posptr[i].x * scale < -0.45 && px != 0) velptr[i].x -= deltaTime / px;
        if (posptr[i].y * scale < -0.45 && py != 0) velptr[i].y -= deltaTime / py;

        float mag = clampf(vec2_mag(velptr[i]), 0.0, 1.0);
        vec2 dir = vec2_normal(velptr[i]);
        velptr[i] = vec2_mult(dir, mag);

        posptr[i].x += velptr[i].x * deltaTime * FLOCK_SPEED;
        posptr[i].y += velptr[i].y * deltaTime * FLOCK_SPEED;
        flockptr[i] = spawn_tri(posptr[i], vec2_to_rad(velptr[i]));

        //Draw
        tri_bind(&flockptr[i]);
        glee_shader_uniform_set(shader_triangle, 4, "u_color", &colptr[i]);
        glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_INT, 0);

    }
}

int main(int argc, char** argv)
{
    rand_seed(time(NULL));
    unsigned int fullscreen = 0, count = rand_next() % 20 + 5;
    
    if (argc > 1) {
        if (!strcmp(argv[1], "-h") || !strcmp(argv[1], "-help")) {
            printf("----------------- FLOCK ------------------\n");
            printf("flock is a simple, interactive screen-saver\n");
            printf("use options -f or -fullscreen for maximum fun.\n");
            printf("\t\tversion 0.1.1\n");
            return EXIT_SUCCESS;
        }
        else if (!strcmp(argv[1], "-f") || !strcmp(argv[1], "-fullscreen")) {
            fullscreen++;
            if (argc > 2) count = atoi(argv[2]);
        } else {
            count = atoi(argv[1]);
            if (!count) count = 2;
        }
    } 

    if (fullscreen) {
        resolution.x = 1440.0;
        resolution.y = 960.0;
    } else {
        resolution.x = (float)SCR_WIDTH;
        resolution.y = (float)SCR_HEIGHT;
    }

    glee_init();
    glee_window_create("flock", (int)resolution.x, (int)resolution.y, fullscreen, 0);
    glee_screen_color(0.0, 0.0, 0.0, 1.0);
    printf("Resolution: %dx%d\n", (int)resolution.x, (int)resolution.y);

    unsigned int quad = glee_buffer_quad_create();
    shader_back = glee_shader_load("shaders/vert.glsl", "shaders/frag.glsl");
    glee_shader_uniform_set(shader_back, 2, "u_resolution", &resolution);
    
    flockId = glee_buffer_id();
    shader_triangle = glee_shader_load("shaders/trivert.glsl", "shaders/trifrag.glsl");
    glee_shader_uniform_set(shader_triangle, 2, "u_resolution", &resolution);
    
    printf("Flock size: %d\n", count);
    vec4 colors[count];
    vec2 positions[count], velocities[count];
    
    posptr = &positions[0], velptr = &velocities[0];
    colptr = &colors[0];
    flockptr = spawn_flock(count);

    float deltaTime, t = glee_time_get();
    while(glee_window_is_open()) {
        glee_screen_clear();
        deltaTime = glee_time_delta(&t);
        
        // Input
        if (glee_key_pressed(GLFW_KEY_ESCAPE)) break;
        if (glee_key_pressed(GLFW_KEY_R)) spawn_flock(count);
        if (glee_key_down(GLFW_KEY_Z)) scale += deltaTime;
        if (glee_key_down(GLFW_KEY_X)) scale -= deltaTime;

        glUseProgram(shader_back);
        glBindVertexArray(quad);
        glee_shader_uniform_set(shader_back, 1, "u_time", &t);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        
        flock_update(count, deltaTime);

        glee_screen_refresh();
    }
    free(flockptr);
    glee_deinit();
    return EXIT_SUCCESS;
}
