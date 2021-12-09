#include "gl-window.h"

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <libzen-compositor/libzen-compositor.h>

static void
GlfwErrorCallback(int code, const char* description)
{
  zen_log("openvr backend - glfw window: %s (%d)\n", description, code);
}

static uint32_t
GetRefreshRate()
{
  // primary_monitor & mode will be freed by glfw
  GLFWmonitor* primary_monitor;
  const GLFWvidmode* mode;

  primary_monitor = glfwGetPrimaryMonitor();
  if (primary_monitor == NULL) {
    zen_log("openvr backend - gl window: failed to get primary monitor\n");
    return 0;
  }

  mode = glfwGetVideoMode(primary_monitor);
  if (mode == NULL) {
    zen_log(
        "openvr backend - gl window: failed to get mode of primary monitor\n");
    return 0;
  }

  return mode->refreshRate * 1000;
}

GlWindow::GlWindow() {}

bool
GlWindow::Init()
{
  GLenum glewError;
  width_ = 640;
  height_ = 320;

  glfwSetErrorCallback(GlfwErrorCallback);

  if (glfwInit() == GLFW_FALSE) {
    zen_log("openvr backend - gl window: failed to initialize glfw\n");
    goto err;
  }

  glfw_window_ = glfwCreateWindow(width_, height_, "zen VR View", NULL, NULL);
  if (glfw_window_ == NULL) {
    zen_log("openvr backend - gl window: failed to create a window\n");
    goto err_window;
  }

  glfwMakeContextCurrent(glfw_window_);
  glfwSwapInterval(0);

  glewError = glewInit();
  if (glewError != GLEW_OK) {
    zen_log("openvr backend - gl window: failed to initialize glew: %s\n",
        glewGetErrorString(glewError));
    goto err_glew;
  }

  refresh_ = GetRefreshRate();
  if (refresh_ == 0) {
    zen_log(
        "openvr backend - gl window: [WARNING] failed to get refresh rate. use "
        "60 fps instead.\n");
    refresh_ = 60000;
  }

  return true;

err_glew:
  glfwDestroyWindow(glfw_window_);

err_window:
  glfwTerminate();

err:
  return false;
}

GlWindow::~GlWindow()
{
  glfwDestroyWindow(glfw_window_);
  glfwTerminate();
}

void
GlWindow::Swap()
{
  glfwSwapBuffers(glfw_window_);
}

bool
GlWindow::Poll()
{
  int32_t width, height;
  glfwPollEvents();

  if (glfwWindowShouldClose(glfw_window_)) return false;
  glfwGetWindowSize(glfw_window_, &width, &height);
  width_ = width;
  height_ = height;

  return true;
}