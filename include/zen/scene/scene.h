#ifndef ZEN_SCENE_H
#define ZEN_SCENE_H

#include <wayland-server-core.h>

#include "zen/scene/screen-layout.h"

struct zn_scene {
  struct zn_screen_layout* screen_layout;

  struct wl_list board_list;  // zn_board::link, non empty
};

struct zn_board* zn_scene_get_focus_board(struct zn_scene* self);

void zn_scene_reassign_boards(struct zn_scene* self);

void zn_scene_setup_bindings(struct zn_scene* self);

struct zn_scene* zn_scene_create(void);

void zn_scene_destroy(struct zn_scene* self);

#endif  //  ZEN_SCENE_H
