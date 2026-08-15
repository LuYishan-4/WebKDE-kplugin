#pragma once
#include "GPUDriverImpl.h"

#include <Ultralight/platform/Config.h>
#include <Ultralight/platform/GPUDriver.h>
#include <memory>
#if defined(_WIN32)
typedef struct GLFWwindow GLFWwindow;
#else
typedef void *EGLDisplay;
typedef void *EGLSurface;
typedef void *EGLContext;
typedef void *EGLImage;
#ifndef GL_OES_EGL_image
typedef void *GLeglImageOES;
typedef void (*PFNGLEGLIMAGETARGETTEXTURE2DOESPROC)(unsigned int target,
                                                    GLeglImageOES image);
#endif
#endif
#define ENABLE_OFFSCREEN_GL 0
namespace ultralight {

class GPUContextGL {
public:
  enum class Mode {
    OwnedOffscreen,
    ExternalCurrent,
  };
  void *ExportTextureAsEGLImage(unsigned int gl_texture_id) const;
  static bool ImportEGLImageIntoTexture(void *image,
                                        unsigned int gl_texture_id);
  void DestroyEGLImage(void *image) const;

protected:
  std::unique_ptr<ultralight::GPUDriverImpl> driver_;
#if defined(_WIN32)
  GLFWwindow *window_ = nullptr;
  GLFWwindow *active_window_ = nullptr;
#endif
  bool msaa_enabled_;
  Mode mode_;
  void *external_context_token_ = nullptr;

public:
  GPUContextGL(bool enable_vsync, bool enable_msaa);
  GPUContextGL(Mode mode, bool enable_vsync, bool enable_msaa);

  virtual ~GPUContextGL() {}

  virtual ultralight::GPUDriverImpl *driver() const { return driver_.get(); }

  virtual ultralight::FaceWinding face_winding() const {
    return ultralight::FaceWinding::CounterClockwise;
  }

  virtual void BeginDrawing() {}

  virtual void EndDrawing() {}

  bool makeCurrent();
  void restoreCurrent();
  void flush();
  bool is_valid() const;
  bool has_current_context() const;
  bool is_glad_ready() const;
  void *current_context_token() const;
  virtual bool msaa_enabled() const { return msaa_enabled_; }
  virtual Mode mode() const { return mode_; }
  virtual bool owns_context() const { return mode_ == Mode::OwnedOffscreen; }
  virtual bool uses_external_context() const {
    return mode_ == Mode::ExternalCurrent;
  }
  virtual void set_external_context_token(void *token) {
    external_context_token_ = token;
  }
#if defined(_WIN32)
  virtual GLFWwindow *window() { return window_; }

  // FBOs are not shared across contexts in OpenGL 3.2 (AFAIK), we luckily
  // don't need to share them across multiple windows anyways so we temporarily
  // set the active GL context to the "active window" when creating FBOs.
  virtual void set_active_window(GLFWwindow *win) { active_window_ = win; }

  virtual GLFWwindow *active_window() { return active_window_; }
#else

  EGLDisplay kwin_egl_display_ = nullptr;
  EGLContext kwin_egl_context_ = nullptr;
  EGLSurface kwin_egl_draw_surface_ = nullptr;
  EGLSurface kwin_egl_read_surface_ = nullptr;

  EGLDisplay egl_display_ = nullptr;
  EGLContext egl_context_ = nullptr;
  EGLSurface egl_surface_ = nullptr;

  EGLContext previous_context_ = nullptr;
  EGLSurface previous_draw_surface_ = nullptr;
  EGLSurface previous_read_surface_ = nullptr;

#endif
};

} // namespace ultralight
