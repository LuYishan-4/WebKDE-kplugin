#pragma once
#include "../config/CursorJSON.hpp"
#include "../config/UserConfig.hpp"
#include "../lib/WebCall/WebCall.hpp"
#include "../lib/gl/GPUContextGL.h"
#include <AppCore/AppCore.h>
#include <Ultralight/Ultralight.h>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>
namespace UltralightWebCursorM {
class UltralightHtmlEffect {
public:
  UltralightHtmlEffect();
  ~UltralightHtmlEffect();
  bool initialize(const ConfigValues &uconfig, const JSONConf &data);
  bool load(const std::string &path);
  void update();
  bool ensureInitialized();
  void move(int x, int y, bool pressed);
  ultralight::View *view() const;
  void reload(const ConfigValues &uconfig, const JSONConf &data);
  bool resize(const int &width, const int &height);
  const uint8_t *pixels() const;
  unsigned int textureId() const;
  // NEW: imports the latest rendered frame (currently held as an EGLImage,
  // produced in Ultralight's own unshared GL context) into a texture that
  // belongs to whichever context is current when this is called. Must be
  // called with the CALLER's context (e.g. KWin's) current, not
  // Ultralight's — that's the whole point of going through EGLImage
  // instead of handing out the raw GL texture id from textureId(), which
  // is only valid inside Ultralight's own context and crashes if bound
  // directly in an unrelated, unshared context.
  bool importFrameIntoTexture(unsigned int dest_gl_texture_id) const;
  int width() const { return html_value_.width_; }
  int height() const { return html_value_.height_; }
  int stride() const { return html_value_.stride_; }
  int hotspotX() const { return html_value_.hotspot_x_; }
  int hotspotY() const { return html_value_.hotspot_y_; }
  void setEnabled(bool enabled);
  bool isEnabled() const;
  bool hasNewFrame() const;
  void clearNewFrame();

private:
  struct Html_Value {
    int width_ = 128;
    int height_ = 128;
    int stride_ = 0;
    int minwidth = 128;
    int minheight = 128;
    int hotspot_x_ = 64;
    int hotspot_y_ = 64;
    std::string m_permanentSdkPath;
    std::filesystem::path html_path_;
    bool use_gpu_ = true;
  };
  Html_Value html_value_;
  std::unique_ptr<ultralight::GPUContextGL> context_;
  ultralight::RefPtr<ultralight::Renderer> renderer_;
  std::shared_ptr<WebCall> webcall;
  ultralight::RefPtr<ultralight::View> view_;
  std::unique_ptr<ultralight::LoadListener> listener_;
  bool is_loaded_ = false;
  bool pending_gpu_init_ = false;
  bool enabled_ = true;
  bool new_frame_ = false;
  bool platform_initialized_ = false;
  std::vector<uint8_t> pixel_buffer_;
  std::filesystem::file_time_type html_time_;
  // NEW: EGLImage wrapping the most recently rendered frame's texture,
  // created in Ultralight's own GL context. Opaque void* to avoid pulling
  // EGL headers into this cross-platform header; cast internally in the
  // .cpp against EGLImage. Owned by this class, destroyed in update()
  // (replaced each frame) and in the destructor.
  void *last_egl_image_ = nullptr;
};
} // namespace UltralightWebCursorM
