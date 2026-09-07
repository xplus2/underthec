#include "scene_internal.h"
#include "color.h"

#include "art/misc.h"

void spawn_splat(struct scene *sc, double tx, double ty, int tz) {
  struct entity *e = entity_spawn(&sc->entities);
  e->type = ENT_SPLAT;
  e->x = tx - 4;
  e->y = ty - 2;
  e->z = tz - 2;
  e->frames = splat_frames;
  e->frame_count = 4;
  e->frame_interval = 2.5; /* .25s per frame */
  e->die_frame = 15;
  e->sentinel = ' ';
  e->default_attr = color_from_name("RED");
}

void spawn_bubble(struct scene *sc, double fish_x, double fish_y, int fish_z, int fish_w, int fish_h, double fish_vx) {
  struct entity *e = entity_spawn(&sc->entities);
  e->type = ENT_BUBBLE;
  e->x = fish_x + (fish_vx > 0 ? fish_w : 0);
  e->y = fish_y + fish_h / 2;
  e->z = fish_z - 1;
  e->vx = 0;
  e->vy = -1;
  e->frames = bubble_frames;
  e->frame_count = 5;
  e->frame_interval = 1.0; /* .1s/frame */
  e->die_offscreen = true;
  e->physical = true;
  e->default_attr = color_from_name("CYAN");
}
