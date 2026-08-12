#include "GPUContextGL.h"
#include "GPUDriverGL.h"
#include "glad/glad.h"
#include <qlogging.h>

#if defined(_WIN32)
#include <GLFW/glfw3.h>
#else
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <QDebug>
#include <cstring>
#endif

namespace ultralight {

#if !defined(_WIN32)
namespace {
bool g_glad_loaded_from_egl = false;

bool LoadGladFromEgl() {
  if (g_glad_loaded_from_egl)
    return true;

  const int result =
      gladLoadGLLoader(reinterpret_cast<GLADloadproc>(eglGetProcAddress));

  g_glad_loaded_from_egl = result != 0;
  qDebug() << "[UltralightCursorEffect]"
           << "GLAD result =" << result << "loaded =" << g_glad_loaded_from_egl;

  if (g_glad_loaded_from_egl && glad_glGetString) {
    const char *version =
        reinterpret_cast<const char *>(glGetString(GL_VERSION));
    const char *vendor = reinterpret_cast<const char *>(glGetString(GL_VENDOR));
    const char *renderer =
        reinterpret_cast<const char *>(glGetString(GL_RENDERER));

    qDebug() << "[UltralightCursorEffect]"
             << "GL_VERSION =" << version << "GL_VENDOR =" << vendor
             << "GL_RENDERER =" << renderer;
  }

  return g_glad_loaded_from_egl;
}

void PrintEglError(const char *where) {
  const EGLint error = eglGetError();
  qDebug() << "[UltralightCursorEffect]" << where << "EGL error =" << Qt::hex
           << error;
}

bool HasEglExtension(EGLDisplay display, const char *extension) {
  const char *extensions = eglQueryString(display, EGL_EXTENSIONS);

  if (!extensions)
    return false;

  return std::strstr(extensions, extension) != nullptr;
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

    if (enable_msaa)
      glfwWindowHint(GLFW_SAMPLES, 4);

    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

    GLFWwindow *win = glfwCreateWindow(10, 10, "", nullptr, nullptr);

    window_ = win;

    if (!window_) {
      glfwTerminate();
      return;
    }

    glfwMakeContextCurrent(window_);

    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
      glfwDestroyWindow(window_);
      window_ = nullptr;
      return;
    }

    glfwSwapInterval(enable_vsync ? 1 : 0);

    GLint samples = 0;
    glGetIntegerv(GL_SAMPLES, &samples);

    if (samples <= 0)
      msaa_enabled_ = false;

    if (msaa_enabled_)
      glEnable(GL_MULTISAMPLE);

#else
    kwin_egl_display_ = eglGetCurrentDisplay();
    kwin_egl_context_ = eglGetCurrentContext();
    kwin_egl_draw_surface_ = eglGetCurrentSurface(EGL_DRAW);
    kwin_egl_read_surface_ = eglGetCurrentSurface(EGL_READ);

    if (kwin_egl_display_ == EGL_NO_DISPLAY ||
        kwin_egl_context_ == EGL_NO_CONTEXT) {

      qDebug() << "[UltralightCursorEffect]"
               << "OwnedOffscreen:"
               << "KWin has no current EGL context";

      return;
    }

    qDebug() << "[UltralightCursorEffect]"
             << "KWin EGL display ="
             << reinterpret_cast<void *>(kwin_egl_display_);

    qDebug() << "[UltralightCursorEffect]"
             << "KWin EGL context ="
             << reinterpret_cast<void *>(kwin_egl_context_);

    egl_display_ = kwin_egl_display_;
    EGLint major = 0;
    EGLint minor = 0;

    if (!eglInitialize(egl_display_, &major, &minor)) {
      PrintEglError("eglInitialize");
      egl_display_ = EGL_NO_DISPLAY;
      return;
    }

    qDebug() << "[UltralightCursorEffect]"
             << "EGL version =" << major << "." << minor;

    const bool hasKHRCreateContext =
        HasEglExtension(egl_display_, "EGL_KHR_create_context");

    if (!hasKHRCreateContext) {
      qWarning() << "[UltralightCursorEffect]"
                 << "EGL_KHR_create_context"
                 << "is not supported";

      return;
    }

    EGLint config_attribs[] = {EGL_SURFACE_TYPE,
                               EGL_PBUFFER_BIT,
                               EGL_RENDERABLE_TYPE,
                               EGL_OPENGL_BIT,
                               EGL_RED_SIZE,
                               8,
                               EGL_GREEN_SIZE,
                               8,
                               EGL_BLUE_SIZE,
                               8,
                               EGL_ALPHA_SIZE,
                               8,
                               EGL_NONE};

    EGLConfig config = nullptr;
    EGLint config_count = 0;

    if (!eglChooseConfig(egl_display_, config_attribs, &config, 1,
                         &config_count) ||
        config_count <= 0) {
      PrintEglError("eglChooseConfig");
      return;
    }

    if (!eglBindAPI(EGL_OPENGL_API)) {
      PrintEglError("eglBindAPI(EGL_OPENGL_API)");
      return;
    }

    EGLint pbuffer_attribs[] = {EGL_WIDTH, 16, EGL_HEIGHT, 16, EGL_NONE};

    egl_surface_ =
        eglCreatePbufferSurface(egl_display_, config, pbuffer_attribs);

    if (egl_surface_ == EGL_NO_SURFACE) {
      PrintEglError("eglCreatePbufferSurface");
      return;
    }

    EGLint context_attribs[] = {
        EGL_CONTEXT_MAJOR_VERSION_KHR,
        3,
        EGL_CONTEXT_MINOR_VERSION_KHR,
        2,
        EGL_CONTEXT_OPENGL_PROFILE_MASK_KHR,
        EGL_CONTEXT_OPENGL_COMPATIBILITY_PROFILE_BIT_KHR,
        EGL_NONE};

    egl_context_ = eglCreateContext(egl_display_, config, kwin_egl_context_,
                                    context_attribs);

    if (egl_context_ == EGL_NO_CONTEXT) {
      PrintEglError("eglCreateContext(OpenGL 3.2)");
      eglDestroySurface(egl_display_, egl_surface_);
      egl_surface_ = EGL_NO_SURFACE;
      return;
    }

    qDebug() << "[UltralightCursorEffect]"
             << "Ultralight EGL 3.2 context created";

    qDebug() << "[UltralightCursorEffect]"
             << "Ultralight EGL context ="
             << reinterpret_cast<void *>(egl_context_);
    previous_context_ = eglGetCurrentContext();
    previous_draw_surface_ = eglGetCurrentSurface(EGL_DRAW);
    previous_read_surface_ = eglGetCurrentSurface(EGL_READ);

    if (!eglMakeCurrent(egl_display_, egl_surface_, egl_surface_,
                        egl_context_)) {

      PrintEglError("eglMakeCurrent(Ultralight)");

      eglDestroyContext(egl_display_, egl_context_);
      egl_context_ = EGL_NO_CONTEXT;

      eglDestroySurface(egl_display_, egl_surface_);
      egl_surface_ = EGL_NO_SURFACE;

      return;
    }

    if (!LoadGladFromEgl()) {
      qWarning() << "[UltralightCursorEffect]"
                 << "Failed to load GLAD";

      restoreCurrent();

      eglDestroyContext(egl_display_, egl_context_);
      egl_context_ = EGL_NO_CONTEXT;

      eglDestroySurface(egl_display_, egl_surface_);
      egl_surface_ = EGL_NO_SURFACE;

      return;
    }

    const char *version =
        reinterpret_cast<const char *>(glGetString(GL_VERSION));
    const char *vendor = reinterpret_cast<const char *>(glGetString(GL_VENDOR));
    const char *renderer =
        reinterpret_cast<const char *>(glGetString(GL_RENDERER));

    qDebug() << "[UltralightCursorEffect]"
             << "Ultralight OpenGL version =" << version;

    qDebug() << "[UltralightCursorEffect]"
             << "Ultralight GL vendor =" << vendor;

    qDebug() << "[UltralightCursorEffect]"
             << "Ultralight GL renderer =" << renderer;

    msaa_enabled_ = false;
    restoreCurrent();
#endif
  }
  driver_.reset(new ultralight::GPUDriverGL(this));
}

#if defined(_WIN32)

bool GPUContextGL::makeCurrent() {
  if (!window_)
    return false;

  glfwMakeContextCurrent(window_);
  return true;
}

void GPUContextGL::restoreCurrent() { glfwMakeContextCurrent(window_); }

void GPUContextGL::flush() { glFlush(); }

bool GPUContextGL::is_valid() const { return window_ != nullptr; }

#else

bool GPUContextGL::makeCurrent() {
  if (mode_ == Mode::ExternalCurrent) {
    return eglGetCurrentContext() != EGL_NO_CONTEXT;
  }

  if (egl_display_ == EGL_NO_DISPLAY || egl_context_ == EGL_NO_CONTEXT ||
      egl_surface_ == EGL_NO_SURFACE) {
    return false;
  }

  previous_context_ = eglGetCurrentContext();
  previous_draw_surface_ = eglGetCurrentSurface(EGL_DRAW);
  previous_read_surface_ = eglGetCurrentSurface(EGL_READ);

  if (!eglMakeCurrent(egl_display_, egl_surface_, egl_surface_, egl_context_)) {

    PrintEglError("eglMakeCurrent");
    return false;
  }

  return true;
}

void GPUContextGL::restoreCurrent() {
  if (previous_context_ == EGL_NO_CONTEXT) {
    return;
  }

  if (!eglMakeCurrent(kwin_egl_display_, previous_draw_surface_,
                      previous_read_surface_, previous_context_)) {

    PrintEglError("restore KWin EGL context");
  }

  previous_context_ = EGL_NO_CONTEXT;
  previous_draw_surface_ = EGL_NO_SURFACE;
  previous_read_surface_ = EGL_NO_SURFACE;
}

void GPUContextGL::flush() {
  if (egl_context_ == EGL_NO_CONTEXT)
    return;

  glFlush();
}

bool GPUContextGL::is_valid() const {
  return egl_display_ != EGL_NO_DISPLAY && egl_context_ != EGL_NO_CONTEXT &&
         egl_surface_ != EGL_NO_SURFACE;
}

#endif

bool GPUContextGL::has_current_context() const {
#if defined(_WIN32)
  return glfwGetCurrentContext() != nullptr;
#else
  if (mode_ == Mode::OwnedOffscreen) {
    return eglGetCurrentContext() == egl_context_;
  }
  return eglGetCurrentContext() != EGL_NO_CONTEXT;
#endif
}

bool GPUContextGL::is_glad_ready() const {
  return glad_glGetString != nullptr && glad_glBindTexture != nullptr;
}

void *GPUContextGL::current_context_token() const {
#if defined(_WIN32)
  if (mode_ == Mode::OwnedOffscreen)
    return reinterpret_cast<void *>(glfwGetCurrentContext());

  return nullptr;
#else
  if (mode_ == Mode::OwnedOffscreen)
    return reinterpret_cast<void *>(egl_context_);

  return reinterpret_cast<void *>(eglGetCurrentContext());
#endif
}
} // namespace ultralight
