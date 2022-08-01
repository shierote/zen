#include "zen/cursor.h"

#include <drm/drm_fourcc.h>
#include <linux/input.h>
#include <wlr/types/wlr_seat.h>
#include <wlr/xcursor.h>

#include "zen-common.h"
#include "zen/scene/screen-layout.h"
#include "zen/server.h"

static void
zn_cursor_handle_add_screen(struct wl_listener* listener, void* data)
{
  struct zn_cursor* self = zn_container_of(listener, self, add_screen_signal);
  if (self->screen == NULL) {
    self->screen = data;
    self->x = self->screen->output->wlr_output->width / 2;
    self->y = self->screen->output->wlr_output->height / 2;
  }
}

void
zn_cursor_set_image(struct zn_cursor* self, const char* name)
{
  struct zn_server* server = zn_server_get_singleton();
  struct wlr_xcursor* xcursor;

  xcursor = wlr_xcursor_manager_get_xcursor(self->xcursor_manager, name, 1.f);
  if (xcursor == NULL) {
    zn_error("Failed to get xcursor (%s)", name);
    return;
  }
  struct wlr_xcursor_image* image = xcursor->images[0];

  self->texture = wlr_texture_from_pixels(server->renderer, DRM_FORMAT_ARGB8888,
      image->width * 4, image->width, image->height, image->buffer);
}

struct zn_cursor*
zn_cursor_create(void)
{
  struct zn_cursor* self;
  self = zalloc(sizeof *self);
  if (self == NULL) {
    zn_error("Failed to allocate memory");
    goto err;
  }

  self->xcursor_manager = wlr_xcursor_manager_create(NULL, 24);
  if (self->xcursor_manager == NULL) {
    zn_error("Failed to create wlr_xcursor");
    goto err_free;
  }
  wlr_xcursor_manager_load(self->xcursor_manager, 1.f);

  self->screen = NULL;
  zn_cursor_set_image(self, "left_ptr");

  struct zn_server* server = zn_server_get_singleton();
  struct zn_screen_layout* screen_layout = server->scene->screen_layout;
  self->add_screen_signal.notify = zn_cursor_handle_add_screen;
  wl_signal_add(&screen_layout->add_screen, &self->add_screen_signal);

  return self;

err_free:
  free(self);

err:
  return NULL;
}

void
zn_cursor_destroy(struct zn_cursor* self)
{
  wl_list_remove(&self->add_screen_signal.link);
  wlr_texture_destroy(self->texture);
  wlr_xcursor_manager_destroy(self->xcursor_manager);
  free(self);
}
