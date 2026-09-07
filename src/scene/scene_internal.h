#ifndef UNDERTHEC_SCENE_INTERNAL_H
#define UNDERTHEC_SCENE_INTERNAL_H

#include "scene.h"

#define Z_SHARK 2
#define Z_TEETH 3
#define Z_FISH_MIN 3
#define Z_FISH_RANGE 17
#define Z_SEAWEED 21
#define Z_CASTLE 22
#define Z_MESSAGE 23
#define Z_SHIP 7
#define Z_WHALE 5
#define Z_MONSTER 5
#define Z_BIGFISH 2
#define Z_SUBMARINE 3
#define Z_SWORDFISH 2
#define Z_DUCK 3
#define Z_DOLPHIN 3
#define Z_SWAN 3
#define Z_FISHHOOK 7
#define Z_HOOKED 5
#define Z_CRAB 21

/* effects.c */
void spawn_splat(struct scene *sc, double tx, double ty, int tz);
void spawn_bubble(struct scene *sc, double fish_x, double fish_y, int fish_z, int fish_w, int fish_h, double fish_vx);

/* creatures.c */
void spawn_fish(struct scene *sc, int w, int h);
void spawn_random_object(struct scene *sc, int w, int h);
void finish_creature_spawn(struct entity *e, enum entity_type type, int z, double vx, double vy, enum death_action death, struct attr attr);
int random_swim_y(int h, int sprite_height);

/* environment.c */
void add_environment(struct scene *sc, int w, int h);
void add_castle(struct scene *sc, int w, int h);
void add_castle_building(struct scene *sc, int w, int h);
void spawn_rubble(struct scene *sc, double castle_x, double castle_y, int castle_height);
void add_seaweed(struct scene *sc, int w, int h);
void add_all_seaweed(struct scene *sc, int w, int h);
void add_all_fish(struct scene *sc, int w, int h);
void add_message(struct scene *sc, int w, int h);
void environment_tick(struct scene *sc);

/* kaiju.c */
void spawn_kaiju(struct scene *sc, int w, int h);
void kaiju_tick(struct scene *sc, int term_w);
void schedule_kaiju_return(struct scene *sc);

/* fishhook.c */
void spawn_fishhook(struct scene *sc, int w, int h);
void fishhook_tick(struct scene *sc, int term_h);

#endif
