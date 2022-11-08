#include "gl-vertex-array.h"

#include <zen-common.h>
#include <zigen-gles-v32-protocol.h>

#include "gl-vertex-attrib.h"

static void zgnr_gl_vertex_array_destroy(
    struct zgnr_gl_vertex_array_impl *self);

static void
zgnr_gl_vertex_array_handle_destroy(struct wl_resource *resource)
{
  struct zgnr_gl_vertex_array_impl *self = wl_resource_get_user_data(resource);
  zgnr_gl_vertex_array_destroy(self);
}

static void
zgnr_gl_vertex_array_protocol_destroy(
    struct wl_client *client, struct wl_resource *resource)
{
  UNUSED(client);
  wl_resource_destroy(resource);
}

static struct zgnr_gl_vertex_attrib *
zgnr_gl_vertex_array_ensure_vertex_attrib(
    struct zgnr_gl_vertex_array_impl *self, uint32_t index)
{
  struct zgnr_gl_vertex_attrib *attrib;
  wl_array_for_each (attrib, &self->pending.vertex_attribs) {
    if (attrib->info.index == index) {
      return attrib;
    }
  }

  attrib = wl_array_add(&self->pending.vertex_attribs, sizeof *attrib);
  zgnr_weak_resource_init(&attrib->gl_buffer);
  attrib->info.enabled = false;
  attrib->data_changed = false;
  attrib->enable_changed = false;

  return attrib;
}

static void
zgnr_gl_vertex_array_protocol_enable_vertex_attrib_array(
    struct wl_client *client, struct wl_resource *resource, uint32_t index)
{
  UNUSED(client);
  struct zgnr_gl_vertex_array_impl *self = wl_resource_get_user_data(resource);
  struct zgnr_gl_vertex_attrib *attrib =
      zgnr_gl_vertex_array_ensure_vertex_attrib(self, index);
  attrib->info.enabled = true;
  attrib->enable_changed = true;
}

static void
zgnr_gl_vertex_array_protocol_disable_vertex_attrib_array(
    struct wl_client *client, struct wl_resource *resource, uint32_t index)
{
  UNUSED(client);
  struct zgnr_gl_vertex_array_impl *self = wl_resource_get_user_data(resource);
  struct zgnr_gl_vertex_attrib *attrib =
      zgnr_gl_vertex_array_ensure_vertex_attrib(self, index);
  attrib->info.enabled = false;
  attrib->enable_changed = true;
}

static void
zgnr_gl_vertex_array_protocol_vertex_attrib_pointer(struct wl_client *client,
    struct wl_resource *resource, uint32_t index, int32_t size, uint32_t type,
    uint32_t normalized, int32_t stride, uint32_t offset,
    struct wl_resource *gl_buffer)
{
  UNUSED(client);
  struct zgnr_gl_vertex_array_impl *self = wl_resource_get_user_data(resource);
  struct zgnr_gl_vertex_attrib *attrib =
      zgnr_gl_vertex_array_ensure_vertex_attrib(self, index);
  attrib->info.size = size;
  attrib->info.type = type;
  attrib->info.normalized = normalized;
  attrib->info.stride = stride;
  attrib->info.offset = offset;
  attrib->data_changed = true;
  zgnr_weak_resource_link(&attrib->gl_buffer, gl_buffer);
}

static const struct zgn_gl_vertex_array_interface implementation = {
    .destroy = zgnr_gl_vertex_array_protocol_destroy,
    .enable_vertex_attrib_array =
        zgnr_gl_vertex_array_protocol_enable_vertex_attrib_array,
    .disable_vertex_attrib_array =
        zgnr_gl_vertex_array_protocol_disable_vertex_attrib_array,
    .vertex_attrib_pointer =
        zgnr_gl_vertex_array_protocol_vertex_attrib_pointer,
};

void
zgnr_gl_vertex_array_commit(struct zgnr_gl_vertex_array_impl *self)
{
  struct zgnr_gl_vertex_attrib *attrib;

  wl_array_for_each (attrib, &self->base.current.vertex_attribs) {
    zgnr_weak_resource_unlink(&attrib->gl_buffer);
  }
  wl_array_release(&self->base.current.vertex_attribs);
  wl_array_init(&self->base.current.vertex_attribs);

  wl_array_for_each (attrib, &self->pending.vertex_attribs) {
    struct zgnr_gl_vertex_attrib *dest;
    dest = wl_array_add(&self->base.current.vertex_attribs, sizeof *dest);

    memcpy(&dest->info, &attrib->info, sizeof attrib->info);

    dest->data_changed = attrib->data_changed;
    dest->enable_changed = attrib->enable_changed;

    zgnr_weak_resource_init(&dest->gl_buffer);
    zgnr_weak_resource_link(&dest->gl_buffer, attrib->gl_buffer.resource);

    attrib->data_changed = false;
    attrib->enable_changed = false;
  }
}

struct zgnr_gl_vertex_array_impl *
zgnr_gl_vertex_array_create(struct wl_client *client, uint32_t id)
{
  struct zgnr_gl_vertex_array_impl *self;
  struct wl_resource *resource;

  self = zalloc(sizeof *self);
  if (self == NULL) {
    zn_error("Failed to allocate memory");
    goto err;
  }

  wl_signal_init(&self->base.events.destroy);
  wl_array_init(&self->base.current.vertex_attribs);
  wl_array_init(&self->pending.vertex_attribs);

  resource = wl_resource_create(client, &zgn_gl_vertex_array_interface, 1, id);
  if (resource == NULL) {
    zn_error("Failed to create a wl_resource");
    goto err_free;
  }

  wl_resource_set_implementation(
      resource, &implementation, self, zgnr_gl_vertex_array_handle_destroy);

  return self;

err_free:
  free(self);

err:
  return NULL;
}

static void
zgnr_gl_vertex_array_destroy(struct zgnr_gl_vertex_array_impl *self)
{
  wl_signal_emit(&self->base.events.destroy, NULL);
  struct zgnr_gl_vertex_attrib *attrib;

  wl_array_for_each (attrib, &self->pending.vertex_attribs) {
    zgnr_weak_resource_unlink(&attrib->gl_buffer);
  }
  wl_array_release(&self->pending.vertex_attribs);

  wl_array_for_each (attrib, &self->base.current.vertex_attribs) {
    zgnr_weak_resource_unlink(&attrib->gl_buffer);
  }
  wl_array_release(&self->base.current.vertex_attribs);

  wl_list_remove(&self->base.events.destroy.listener_list);
  free(self);
}
