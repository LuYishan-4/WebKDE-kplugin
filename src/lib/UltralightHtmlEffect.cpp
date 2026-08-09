#include "../header/UltralightHtmlEffect.hpp"

#include "UltralightPl/WebListener.hpp"
#include "gl/GPUDriverGL.h"
#include <AppCore/Platform.h>
#include <QDBusConnection>
#include <QDebug>
#include <QProcess>
#include <Ultralight/Ultralight.h>
#include <cstring>
#include <fstream>
#include <iostream>
#include <memory>

namespace UltralightWebCursorM {
UltralightHtmlEffect::UltralightHtmlEffect() {}

UltralightHtmlEffect::~UltralightHtmlEffect() {
  std::cout << "[Ultralight] destroy\n";
  listener_.reset();
  view_ = nullptr;
  renderer_ = nullptr;
  webcall = nullptr;
  context_ = nullptr;
}

// initialize
bool UltralightHtmlEffect::initialize(const ConfigValues &uconfig,
                                      const JSONConf &data) {
  html_value_ = {.width_ = data.minWidth,
                 .height_ = data.minHeight,
                 .stride_ = 0,
                 .minwidth = data.minWidth,
                 .minheight = data.minHeight,
                 .hotspot_x_ = data.hotspotX,
                 .hotspot_y_ = data.hotspotY,
                 .m_permanentSdkPath = uconfig.sdk,
                 .html_path_ = uconfig.html,
                 .use_gpu_ = uconfig.EnableGPU};
  pending_gpu_init_ = html_value_.use_gpu_;
  std::filesystem::path sdk_dir(html_value_.m_permanentSdkPath);
  std::filesystem::path resources_dir = sdk_dir / "resources";
  if (!std::filesystem::exists(resources_dir))
    return false;

  std::vector<std::string> required_files = {"cacert.pem", "icudt67l.dat"};

  for (const auto &file_name : required_files)
    if (!std::filesystem::exists(resources_dir / file_name))
      return false;
  if (!platform_initialized_) {
    ultralight::Config config;
    config.resource_path_prefix = ultralight::String("resources/");
    auto &platform = ultralight::Platform::instance();
    platform.set_config(config);
    platform.set_font_loader(ultralight::GetPlatformFontLoader());
    platform.set_file_system(ultralight::GetPlatformFileSystem(
        ultralight::String(html_value_.m_permanentSdkPath.c_str())));
    platform_initialized_ = true;
  }

  if (std::filesystem::exists(html_value_.html_path_))
    html_time_ = std::filesystem::last_write_time(html_value_.html_path_);

  return true;
}

bool UltralightHtmlEffect::ensureInitialized() {
  if (renderer_ && view_)
    return true;

  auto &platform = ultralight::Platform::instance();
  if (pending_gpu_init_ && !context_) {
    qDebug() << "[UltralightCursorEffect] intiglglglglg";
    context_ = std::make_unique<ultralight::GPUContextGL>(
        ultralight::GPUContextGL::Mode::ExternalCurrent, false, false);
  }

  if (pending_gpu_init_ && context_ && !context_->is_glad_ready()) {
    qDebug() << "[UltralightCursorEffect] glad not initialized on current "
                "Linux/KWin path";
  }

  if (pending_gpu_init_ && context_ && !context_->has_current_context()) {
    qDebug() << "[UltralightCursorEffect] GPU init deferred: no current GL "
                "context yet";
    return false;
  }

  if (pending_gpu_init_ && context_) {
    platform.set_gpu_driver(context_->driver());
    context_->set_external_context_token(context_.get());
    const auto *glVersion =
        reinterpret_cast<const char *>(glGetString(GL_VERSION));
    qDebug() qDebug()
        << "[UltralightCursorEffect] GPU init with current context, GL_VERSION="
        << (glVersion ? glVersion : "<null>")
        << " | glad ready:" << context_->is_glad_ready();
  }

  qDebug() << "[UltralightCursorEffect] init4" << html_value_.html_path_.c_str()
           << html_value_.m_permanentSdkPath.c_str();
  renderer_ = ultralight::Renderer::Create();
  if (!renderer_)
    return false;

  ultralight::ViewConfig vc;
  vc.is_accelerated = pending_gpu_init_;
  vc.is_transparent = true;
  vc.enable_images = true;
  vc.enable_javascript = true;
  view_ = renderer_->CreateView(html_value_.width_, html_value_.height_, vc,
                                nullptr);
  if (!view_)
    return false;

  listener_ = std::make_unique<LocalLoadListener>(&is_loaded_);
  view_->set_load_listener(listener_.get());
  webcall = std::make_shared<WebCall>();
  webcall->view_ = view_;
  qDebug() << "[UltralightCursorEffect] 3";
  return load(html_value_.html_path_);
}

bool UltralightHtmlEffect::load(const std::string &path) {
  qDebug() << "[UltralightCursorEffect] 4";
  std::ifstream file(path);
  if (!file) {
    qDebug() << "[UltralightCursorEffect] Failed to open file:"
             << QString::fromStdString(path);
    return false;
  }
  qDebug() << "[UltralightCursorEffect] 5";
  std::filesystem::path p(path);
  std::string folderName = p.parent_path().filename().string();
  std::string fileUrl = "file:///" + folderName + "/index.html";
  is_loaded_ = false;
  qDebug() << "[UltralightCursorEffect] 6";
  view_->LoadURL(fileUrl.c_str());
  qDebug() << "[UltralightCursorEffect] 7";
  view_->set_needs_paint(true);
  return true;
}

bool UltralightHtmlEffect::resize(const int &width, const int &height) {
  if (width > html_value_.minwidth || height > html_value_.minheight)
    return false;
  view_->Resize(width, height);
  return true;
}

void UltralightHtmlEffect::reload(const ConfigValues &uconfig,
                                  const JSONConf &data) {
  html_value_ = {.width_ = data.minWidth,
                 .height_ = data.minHeight,
                 .stride_ = 0,
                 .minwidth = data.minWidth,
                 .minheight = data.minHeight,
                 .hotspot_x_ = data.hotspotX,
                 .hotspot_y_ = data.hotspotY,
                 .m_permanentSdkPath = uconfig.sdk,
                 .html_path_ = uconfig.html,
                 .use_gpu_ = uconfig.EnableGPU};
  UltralightHtmlEffect::load(html_value_.html_path_);
  UltralightHtmlEffect::resize(html_value_.width_, html_value_.height_);
}
void UltralightHtmlEffect::move(int x, int y, bool pressed) {
  if (!view_)
    return;
  if (webcall)
    webcall->move(x, y, pressed);
  view_->set_needs_paint(true);
}

void UltralightHtmlEffect::update() {
  qDebug() << "[UltralightCursorEffect] update() called";
  if (!enabled_)
    return;
  if (!ensureInitialized())
    return;
  if (!renderer_ || !view_)
    return;

  static int updateLoopCounter = 0;
  updateLoopCounter++;

  if (!view_->needs_paint() && updateLoopCounter % 15 == 0) {
    view_->set_needs_paint(true);
  }

  renderer_->Update();

  bool target_wants_paint = view_->needs_paint();

  renderer_->Render();

  if (context_) {
    const auto render_target = view_->render_target();
    new_frame_ = target_wants_paint && render_target.texture_id != 0;
    if (updateLoopCounter % 60 == 0) {
      const unsigned int ultralightTextureId = render_target.texture_id;
      const unsigned int resolvedTextureId = textureId();
      const bool isGlTexture =
          resolvedTextureId != 0 && glIsTexture(resolvedTextureId);

      qDebug() << "[UltralightHtmlDebug] [GPU Pulse]"
               << " UL texture id:" << ultralightTextureId
               << " | resolved GL texture id:" << resolvedTextureId
               << " | glIsTexture:" << isGlTexture
               << " | needs_paint():" << target_wants_paint;
    }
  } else {
    auto surface = view_->surface();
    if (!surface)
      return;
    auto bitmap_surface = dynamic_cast<ultralight::BitmapSurface *>(surface);
    if (!bitmap_surface)
      return;
    auto bitmap = bitmap_surface->bitmap();
    if (!bitmap)
      return;

    bitmap->LockPixels();
    uint8_t *raw = static_cast<uint8_t *>(bitmap->raw_pixels());
    if (raw) {
      html_value_.width_ = bitmap->width();
      html_value_.height_ = bitmap->height();
      html_value_.stride_ = bitmap->row_bytes();
      const size_t size = html_value_.stride_ * html_value_.height_;

      pixel_buffer_.resize(size);
      memcpy(pixel_buffer_.data(), raw, size);
      new_frame_ = true;

      if (updateLoopCounter % 60 == 0) {
        qDebug() << "[UltralightHtmlDebug] [CPU Memory Map Pulse] Mapped Pixel "
                    "Size Buffer:"
                 << size
                 << "bytes | Stride bounds alignment:" << html_value_.stride_;
      }
    }
    bitmap->UnlockPixels();
  }
}

void UltralightHtmlEffect::setEnabled(bool enabled) {
  qDebug() << "[UltralightHtmlDebug] Core active status update toggled to:"
           << enabled;
  enabled_ = enabled;
}

bool UltralightHtmlEffect::isEnabled() const { return enabled_; }

bool UltralightHtmlEffect::hasNewFrame() const { return new_frame_; }

void UltralightHtmlEffect::clearNewFrame() { new_frame_ = false; }

const uint8_t *UltralightHtmlEffect::pixels() const {
  if (pixel_buffer_.empty())
    return nullptr;
  return pixel_buffer_.data();
}

unsigned int UltralightHtmlEffect::textureId() const {
  if (!context_ || !view_)
    return 0;

  auto *driver = dynamic_cast<ultralight::GPUDriverGL *>(context_->driver());
  if (!driver)
    return 0;

  const auto render_target = view_->render_target();
  if (render_target.texture_id == 0)
    return 0;

  const unsigned int resolved =
      driver->GetGLTextureId(render_target.texture_id);
  return glIsTexture(resolved) ? resolved : 0;
}

ultralight::View *UltralightHtmlEffect::view() const { return view_.get(); }

} // namespace UltralightWebCursorM
