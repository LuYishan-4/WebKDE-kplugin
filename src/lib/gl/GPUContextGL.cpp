#include "GPUContextGL.h"
#include "GPUDriverGL.h"
#include "glad/glad.h"
#if defined(_WIN32)
#include <GLFW/glfw3.h>
#else
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <QDebug>
#endif

namespace ultralight {

#if !defined(_WIN32)
namespace {
bool g_glad_loaded_from_egl = false;

void EnsureGladLoadedFromEgl() {
  if (g_glad_loaded_from_egl)
    return;

  const int glad_result = gladLoadGLLoader((GLADloadproc)eglGetProcAddress);
  g_glad_loaded_from_egl = glad_result != 0;
  qDebug() << "[UltralightCursorEffect] gladLoadGLLoader(EGL) result="
           << glad_result << " | loaded=" << g_glad_loaded_from_egl
           << " | GLVersion=" << GLVersion.major << "." << GLVersion.minor
           << " | glad_glGetString set=" << (glad_glGetString != nullptr)
           << " | glad_glBindTexture set=" << (glad_glBindTexture != nullptr);
}
} // namespace
#endif

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
    EGLDisplay egl_dpy = eglGetCurrentDisplay();
    EGLConfig egl_cfg = nullptr;

    EGLint config_attribs[] = {EGL_SURFACE_TYPE, EGL_PBUFFER_BIT,
                               EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT, EGL_NONE};
    EGLint num_configs = 0;
    eglChooseConfig(egl_dpy, config_attribs, &egl_cfg, 1, &num_configs);

    EGLint context_attribs[] = {EGL_CONTEXT_MAJOR_VERSION,
                                3,
                                EGL_CONTEXT_MINOR_VERSION,
                                2,
                                EGL_CONTEXT_OPENGL_PROFILE_MASK,
                                EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT,
                                EGL_NONE};

    EGLContext kwin_ctx = eglGetCurrentContext();

    EGLContext my_ctx =
        eglCreateContext(egl_dpy, egl_cfg, kwin_ctx, context_attribs);

    if (my_ctx != EGL_NO_CONTEXT) {
      EGLint pbuffer_attribs[] = {EGL_WIDTH, 10, EGL_HEIGHT, 10, EGL_NONE};
      EGLSurface my_surf =
          eglCreatePbufferSurface(egl_dpy, egl_cfg, pbuffer_attribs);

      eglMakeCurrent(egl_dpy, my_surf, my_surf, my_ctx);

      external_context_token_ = reinterpret_cast<void *>(my_ctx);
    } else {
      qDebug() << "[Ultralight] Failded build";
      external_context_token_ = this;
    }

    msaa_enabled_ = false;
#endif
  }

  driver_.reset(new ultralight::GPUDriverGL(this));
}

bool GPUContextGL::has_current_context() const {
#if defined(_WIN32)
  return glfwGetCurrentContext() != nullptr;
#else
  EnsureGladLoadedFromEgl();
  return glad_glGetString != nullptr && glGetString(GL_VERSION) != nullptr;
#endif
}

bool GPUContextGL::is_glad_ready() const {
#if defined(_WIN32)
  return glad_glGetString != nullptr && glad_glBindTexture != nullptr;
#else
  EnsureGladLoadedFromEgl();
  return glad_glGetString != nullptr && glad_glBindTexture != nullptr;
#endif
}

void *GPUContextGL::current_context_token() const {
  if (mode_ == Mode::ExternalCurrent) {
    return reinterpret_cast<void *>(eglGetCurrentContext());
  }
#if defined(_WIN32)
  return reinterpret_cast<void *>(glfwGetCurrentContext());
#else
  return external_context_token_;
#endif
}

} // namespace ultralight
