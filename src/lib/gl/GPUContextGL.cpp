#include "GPUContextGL.h"
#include "GPUDriverGL.h"
#include "glad/glad.h"
#if defined(_WIN32)
#include <GLFW/glfw3.h>
#endif

namespace ultralight {

GPUContextGL::GPUContextGL(bool enable_vsync, bool enable_msaa)
    : GPUContextGL(Mode::OwnedOffscreen, enable_vsync, enable_msaa) {}

GPUContextGL::GPUContextGL(Mode mode, bool enable_vsync, bool enable_msaa)
    : msaa_enabled_(enable_msaa), mode_(mode) {
#if !defined(_WIN32)
  (void)enable_vsync;
#endif
  if (mode_ == Mode::OwnedOffscreen) {
#if defined(_WIN32)
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#endif

    if (enable_msaa) {
      glfwWindowHint(GLFW_SAMPLES, 4);
    }

    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    GLFWwindow *win = glfwCreateWindow(10, 10, "", NULL, NULL);
    window_ = win;
    if (!window_) {
      glfwTerminate();
      exit(EXIT_FAILURE);
    }

    glfwMakeContextCurrent(window_);
    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
    glfwSwapInterval(enable_vsync ? 1 : 0);

    int samples = 4;
    glGetIntegerv(GL_SAMPLES, &samples);
    if (!samples) {
      msaa_enabled_ = false;
    }

    if (msaa_enabled_) {
      glEnable(GL_MULTISAMPLE);
    }
#else
    external_context_token_ = this;
    msaa_enabled_ = false;
#endif
  }

  driver_.reset(new ultralight::GPUDriverGL(this));
}

bool GPUContextGL::has_current_context() const {
#if defined(_WIN32)
  return glfwGetCurrentContext() != nullptr;
#else
  return glGetString(GL_VERSION) != nullptr;
#endif
}

void *GPUContextGL::current_context_token() const {
  if (mode_ == Mode::ExternalCurrent) {
    return external_context_token_ ? external_context_token_
                                   : const_cast<GPUContextGL *>(this);
  }
#if defined(_WIN32)
  return reinterpret_cast<void *>(glfwGetCurrentContext());
#else
  return const_cast<GPUContextGL *>(this);
#endif
}

} // namespace ultralight
