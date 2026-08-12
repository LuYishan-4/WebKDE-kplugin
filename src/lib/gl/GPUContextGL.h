#pragma once

#include <memory>
#define ENABLE_OFFSCREEN_GL 0
#if !defined(_WIN32)
#include <EGL/egl.h>
#endif

namespace ultralight {

class GPUDriverGL;

class GPUContextGL {
public:
  enum class Mode { OwnedOffscreen, ExternalCurrent };

  GPUContextGL(bool enable_vsync = false, bool enable_msaa = false);

  GPUContextGL(Mode mode, bool enable_vsync, bool enable_msaa);

  ~GPUContextGL();

  bool makeCurrent();

  void restoreCurrent();

  void restore_current();

  void flush();

  bool is_valid() const;

  bool has_current_context() const;

  bool is_glad_ready() const;

  void *current_context_token() const;

  GPUDriverGL *driver() const { return driver_.get(); }

  void BeginDrawing();

  void EndDrawing();

private:
  bool msaa_enabled_ = false;

  Mode mode_ = Mode::OwnedOffscreen;

  std::unique_ptr<GPUDriverGL> driver_;

#if defined(_WIN32)

  GLFWwindow *window_ = nullptr;

#else

  /*
   * ---------------------------------------------------------
   * KWin context
   * ---------------------------------------------------------
   */

  EGLDisplay kwin_egl_display_ = EGL_NO_DISPLAY;

  EGLContext kwin_egl_context_ = EGL_NO_CONTEXT;

  EGLSurface kwin_egl_draw_surface_ = EGL_NO_SURFACE;

  EGLSurface kwin_egl_read_surface_ = EGL_NO_SURFACE;

  /*
   * ---------------------------------------------------------
   * Ultralight context
   * ---------------------------------------------------------
   */

  EGLDisplay egl_display_ = EGL_NO_DISPLAY;

  EGLContext egl_context_ = EGL_NO_CONTEXT;

  EGLSurface egl_surface_ = EGL_NO_SURFACE;

  /*
   * ---------------------------------------------------------
   * Context saved before switching to Ultralight
   * ---------------------------------------------------------
   */

  EGLContext previous_context_ = EGL_NO_CONTEXT;

  EGLSurface previous_draw_surface_ = EGL_NO_SURFACE;

  EGLSurface previous_read_surface_ = EGL_NO_SURFACE;

#endif
};

} // namespace ultralight
