#include "gl-program.h"

#include <zen-common.h>
#include <zgnr/program-shader.h>

#include "gl-shader.h"

static void zna_gl_program_destroy(struct zna_gl_program *self);

static void
zna_gl_program_handle_session_destroy(struct wl_listener *listener, void *data)
{
  struct zna_gl_program *self =
      zn_container_of(listener, self, session_destroy_listener);
  UNUSED(data);

  if (self->znr_gl_program) {
    znr_gl_program_destroy(self->znr_gl_program);
    self->znr_gl_program = NULL;
  }
}

static void
zna_gl_program_handle_zgnr_gl_program_destroy(
    struct wl_listener *listener, void *data)
{
  struct zna_gl_program *self =
      zn_container_of(listener, self, zgnr_gl_program_destroy_listener);
  UNUSED(data);

  zna_gl_program_destroy(self);
}

void
zna_gl_program_apply_commit(struct zna_gl_program *self, bool only_damaged)
{
  struct znr_session *session = self->system->current_session;
  struct zgnr_program_shader *program_shader;

  if (self->znr_gl_program == NULL) {
    self->znr_gl_program = znr_gl_program_create(session);
  }

  wl_list_for_each (program_shader,
      &self->zgnr_gl_program->current.program_shader_list, link) {
    struct zna_gl_shader *shader = program_shader->shader->user_data;
    zna_gl_shader_apply_commit(shader, only_damaged);
  }

  if (self->zgnr_gl_program->current.should_link || !only_damaged) {
    wl_list_for_each (program_shader,
        &self->zgnr_gl_program->current.program_shader_list, link) {
      struct zna_gl_shader *shader = program_shader->shader->user_data;
      znr_gl_program_attach_shader(self->znr_gl_program, shader->znr_gl_shader);
    }
    znr_gl_program_link(self->znr_gl_program);
  }
}

struct zna_gl_program *
zna_gl_program_create(
    struct zgnr_gl_program *zgnr_gl_program, struct zna_system *system)
{
  struct zna_gl_program *self;

  self = zalloc(sizeof *self);
  if (self == NULL) {
    zn_error("Failed to allocate memory");
    goto err;
  }

  self->zgnr_gl_program = zgnr_gl_program;
  zgnr_gl_program->user_data = self;
  self->system = system;
  self->znr_gl_program = NULL;

  self->zgnr_gl_program_destroy_listener.notify =
      zna_gl_program_handle_zgnr_gl_program_destroy;
  wl_signal_add(&self->zgnr_gl_program->events.destroy,
      &self->zgnr_gl_program_destroy_listener);

  self->session_destroy_listener.notify = zna_gl_program_handle_session_destroy;
  wl_signal_add(&self->system->events.current_session_destroyed,
      &self->session_destroy_listener);

  return self;

err:
  return NULL;
}

static void
zna_gl_program_destroy(struct zna_gl_program *self)
{
  if (self->znr_gl_program) znr_gl_program_destroy(self->znr_gl_program);
  wl_list_remove(&self->zgnr_gl_program_destroy_listener.link);
  wl_list_remove(&self->session_destroy_listener.link);
  free(self);
}
