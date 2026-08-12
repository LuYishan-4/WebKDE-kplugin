#include "../header/QtCursorEffect.hpp"
#include <QDebug>
#include <QGuiApplication>
#include <QTimer>

int main(int argc, char **argv) {
  QGuiApplication app(argc, argv);

  UltralightWebCursorM::QtCursorEffect effect;
  if (!effect.initialize()) {
    qCritical() << "Failed to initialize Niri standalone effect";
    return 1;
  }
  effect.start();
  qDebug() << "standalone cursor effect running";
  return app.exec();
}
