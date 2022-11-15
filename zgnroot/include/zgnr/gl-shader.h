#pragma once

#include <wayland-server.h>
#include <zen-common/weak-resource.h>

#ifdef __cplusplus
extern "C" {
#endif

struct zgnr_gl_shader {
  struct {
    struct wl_signal destroy;
  } events;

  struct zgnr_shm_buffer *buffer;  // nonnull
  uint32_t type;

  void *user_data;
};

#ifdef __cplusplus
}
#endif
