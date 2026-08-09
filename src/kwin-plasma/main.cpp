#include "../header/KwinCursorEffect.hpp"
#include "../header/KwinMouseProvider.hpp"
#include "core/rendertarget.h"
#include "core/renderviewport.h"
#include "effect/effecthandler.h"
#include "opengl/glutils.h"
#include <QDBusConnection>
#include <QImage>
#include <iostream>
#include <stdexcept>
#include <thread>
namespace KWin {

extern EffectsHandler *effects;

KWIN_EFFECT_FACTORY_SUPPORTED(KWin::KwinCursorEffect, "metadata.json",
                              return KWin::KwinCursorEffect::supported();)

KwinCursorEffect::KwinCursorEffect() {
  qDebug() << "[UltralightCursorEffect] KwinCursorEffect ctor: begin";
  if (!initializeCore<KwinMouseProvider>())
    return;
  qDebug()
      << "[UltralightCursorEffect] KwinCursorEffect ctor: core initialized";
  connect(effects, &EffectsHandler::windowActivated, this,
          &KwinCursorEffect::slotWindowStateChanged);
  qDebug() << "[UltralightCursorEffect] KwinCursorEffect ctor: windowActivated "
              "connected";
  m_mouseProvider->setCallback(
      [this](const UltralightWebCursorM::MousePoint &pt) {
        if (!m_html)
          return;
        QRect oldRect =
            getCursorRect(m_cursorPoint).toRect().adjusted(-20, -20, 20, 20);
        m_cursorPoint = QPointF(pt.x, pt.y);

        m_html->move(pt.x, pt.y, pt.pressed);

        QRect newRect =
            getCursorRect(m_cursorPoint).toRect().adjusted(-20, -20, 20, 20);
        effects->addRepaint(KWin::Rect(oldRect));
        effects->addRepaint(KWin::Rect(newRect));
      });
  qDebug()
      << "[UltralightCursorEffect] KwinCursorEffect ctor: mouse callback set";
  QDBusConnection::sessionBus().registerObject(
      QStringLiteral("/UltralightCursor"), this,
      QDBusConnection::ExportAllSlots);
  qDebug() << "[UltralightCursorEffect] KwinCursorEffect ctor: dbus object "
              "registered";
  // m_html->update();
}

KwinCursorEffect::~KwinCursorEffect() {
  if (m_mouseProvider) {
    m_mouseProvider->setCallback(nullptr);
    m_mouseProvider.reset();
  }
  m_cursorTexture.reset();
}

bool KwinCursorEffect::supported() { return effects->isOpenGLCompositing(); }

void KwinCursorEffect::enable() {
  UltralightWebCursorM::MainCursorStaff::enable();
  effects->addRepaintFull();
}

void KwinCursorEffect::disable() {
  UltralightWebCursorM::MainCursorStaff::disable();
  m_cursorTexture.reset();
  effects->addRepaintFull();
}

void KwinCursorEffect::reloadHtml() {
  UltralightWebCursorM::MainCursorStaff::reloadHtml();
  effects->addRepaintFull();
}

bool KwinCursorEffect::isBlacklisted() const {
  auto window = effects->activeWindow();
  if (!window)
    return false;
  return isWindowBlacklisted(window->windowClass().toStdString());
}
GLTexture *KwinCursorEffect::ensureCursorTexture() {
  static bool logged = false;
  if (!logged) {
    qDebug() << "[UltralightCursorEffect] ensureCursorTexture() entered"
             << " | glad_glGetString set:" << (glad_glGetString != nullptr)
             << " | glad_glBindTexture set:" << (glad_glBindTexture != nullptr);
    logged = true;
  }
  if (!m_html || !m_html->isEnabled() || m_isIdleHidden)
    return nullptr;

  static bool first_focus_done = false;
  if (!first_focus_done && m_html->view()) {
    m_html->view()->Focus();
    first_focus_done = true;
  }

  GLint native_kwin_fbo = 0;
  GLint native_active_program = 0;
  GLint native_vertex_array = 0;
  GLint native_active_texture = 0;
  GLint native_texture_binding_2d = 0;
  GLint native_viewport[4] = {0, 0, 0, 0};
  GLboolean native_blend = glIsEnabled(GL_BLEND);
  GLboolean native_scissor = glIsEnabled(GL_SCISSOR_TEST);
  glGetIntegerv(GL_FRAMEBUFFER_BINDING, &native_kwin_fbo);
  glGetIntegerv(GL_CURRENT_PROGRAM, &native_active_program);
  glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &native_vertex_array);
  glGetIntegerv(GL_ACTIVE_TEXTURE, &native_active_texture);
  glGetIntegerv(GL_TEXTURE_BINDING_2D, &native_texture_binding_2d);
  glGetIntegerv(GL_VIEWPORT, native_viewport);
  m_html->update();

  glActiveTexture(native_active_texture);
  glBindTexture(GL_TEXTURE_2D, native_texture_binding_2d);
  glBindVertexArray(native_vertex_array);
  glUseProgram(native_active_program);
  glBindFramebuffer(GL_FRAMEBUFFER, native_kwin_fbo);
  glViewport(native_viewport[0], native_viewport[1], native_viewport[2],
             native_viewport[3]);
  if (native_blend)
    glEnable(GL_BLEND);
  else
    glDisable(GL_BLEND);
  if (native_scissor)
    glEnable(GL_SCISSOR_TEST);
  else
    glDisable(GL_SCISSOR_TEST);

  int w = m_html->width();
  int h = m_html->height();
  if (w <= 0 || h <= 0)
    return nullptr;

  if (m_html->view() && m_html->view()->needs_paint()) {
    QRect repaintRect =
        getCursorRect(effects->cursorPos()).toRect().adjusted(-20, -20, 20, 20);
    effects->addRepaint(KWin::Rect(repaintRect));
  }

  unsigned int gpuTexId = m_html->textureId();
  if (gpuTexId != 0) {
    if (!m_cursorTexture || m_lastGpuTexId != gpuTexId ||
        m_cursorTexture->width() != w || m_cursorTexture->height() != h) {
      m_cursorTexture.reset();
      m_cursorTexture =
          GLTexture::createNonOwningWrapper(gpuTexId, GL_RGBA8, QSize(w, h));
      if (!m_cursorTexture)
        return nullptr;
      m_cursorTexture->setWrapMode(GL_CLAMP_TO_EDGE);
      m_cursorTexture->setFilter(GL_LINEAR);
      m_lastGpuTexId = gpuTexId;
    }
    return m_cursorTexture.get();
  }

  if (m_cursorTexture && !m_html->hasNewFrame())
    return m_cursorTexture.get();

  const uint8_t *pixels = m_html->pixels();
  if (!pixels)
    return nullptr;
  if (m_cursorTexture &&
      (m_cursorTexture->width() != w || m_cursorTexture->height() != h))
    m_cursorTexture.reset();
  if (!m_cursorTexture) {
    QImage wrapperImage(const_cast<uint8_t *>(pixels), w, h, m_html->stride(),
                        QImage::Format_ARGB32_Premultiplied);
    m_cursorTexture = GLTexture::upload(wrapperImage);
    if (!m_cursorTexture)
      return nullptr;
    m_cursorTexture->setWrapMode(GL_CLAMP_TO_EDGE);
    m_html->clearNewFrame();
    return m_cursorTexture.get();
  }

  if (m_html->hasNewFrame()) {
    m_cursorTexture->bind();
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w, h, GL_BGRA, GL_UNSIGNED_BYTE,
                    pixels);
    m_cursorTexture->unbind();
    m_html->clearNewFrame();
  }
  return m_cursorTexture.get();
}
void KwinCursorEffect::paintScreen(const RenderTarget &renderTarget,
                                   const RenderViewport &viewport, int mask,
                                   const Region &region,
                                   LogicalOutput *screen) {
  effects->paintScreen(renderTarget, viewport, mask, region, screen);
  if (!m_html || !m_html->isEnabled() || m_isIdleHidden)
    return;

  GLTexture *texture = ensureCursorTexture();
  static int frameCounter = 0;
  frameCounter++;

  if (frameCounter % 60 == 0) {
    unsigned int gpuTexId = m_html->textureId();

    qDebug() << "[UltralightKwinLinkDebug] [KWin Pipeline Context Check]"
             << " | Wrapped Texture ID:" << gpuTexId
             << " | glIsTexture:" << (gpuTexId != 0 && glIsTexture(gpuTexId))
             << " | Wrapped Status:" << (texture != nullptr);
  }

  if (!texture) {
    effects->addRepaintFull();
    return;
  }
  const int w = m_html->width();
  const int h = m_html->height();

  QPointF hotspot(m_html->hotspotX(), m_html->hotspotY());
  QPointF pos = effects->cursorPos() - screen->geometry().topLeft() - hotspot;

  auto scale = viewport.scale();
  QMatrix4x4 mvp = viewport.projectionMatrix();
  mvp.translate(pos.x() * scale, pos.y() * scale);
  ShaderBinder binder(ShaderTrait::MapTexture);
  GLShader *shader = binder.shader();
  if (!shader)
    return;

  shader->setUniform(GLShader::Mat4Uniform::ModelViewProjectionMatrix, mvp);

  glEnablei(GL_BLEND, 0);
  glBlendFunci(0, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
  texture->render(QSizeF(w, h) * scale);
  glDisablei(GL_BLEND, 0);

  if (m_html->view() && m_html->view()->needs_paint()) {
    QRect repaintRect =
        getCursorRect(effects->cursorPos()).toRect().adjusted(-20, -20, 20, 20);
    effects->addRepaint(KWin::Rect(repaintRect));
  }
}

bool KwinCursorEffect::isActive() const { return m_html != nullptr; }

bool KwinCursorEffect::checkFullScreen() const {
  if (EffectWindow *activeWin = effects->activeWindow()) {
    return activeWin->isFullScreen();
  }
  return false;
}

void KwinCursorEffect::slotWindowStateChanged(EffectWindow *w) {
  Q_UNUSED(w);
  if (!checkFullScreen()) {
    if (m_isIdleHidden) {
      m_isIdleHidden = false;
      if (m_html)
        m_html->setEnabled(true);
      effects->addRepaintFull();
    }
  } else {
    m_isIdleHidden = true;
    if (m_html)
      m_html->setEnabled(false);
    effects->addRepaintFull();
  }
}

} // namespace KWin

#include "main.moc"
