#include "SettingsBackendX11.hpp"
#include <QDir>
#include <QDebug>
#include <filesystem>

using namespace UltralightWebCursorM;

SettingsBackendX11::SettingsBackendX11(QObject *parent)
    : SettingsBackend(parent) {}

bool SettingsBackendX11::uploadTheme(const QString &path) {
  QDir srcDir(QDir::cleanPath(path));
  if (!srcDir.exists()) {
    // Reuses protected setStatusMessage() via public save()/reload() path is
    // not available since it's private in the base; emit through the base
    // API instead.
    qWarning() << "[SettingsBackendX11] Theme folder not found:" << path;
    return false;
  }

  QString name = srcDir.dirName();
  std::filesystem::path destPath =
      std::filesystem::path(g_sdkInitialPath) / name.toStdString();

  std::error_code ec;
  std::filesystem::copy(
      srcDir.absolutePath().toStdString(), destPath,
      std::filesystem::copy_options::recursive |
          std::filesystem::copy_options::overwrite_existing,
      ec);

  if (ec) {
    qWarning() << "[SettingsBackendX11] Theme copy failed:"
               << QString::fromStdString(ec.message());
    return false;
  }

  // Base class keeps themeList_ private; trigger a full reload so the
  // freshly copied theme shows up.
  reload();
  return true;
}

bool SettingsBackendX11::removeTheme(const QString &name) {
  if (name.isEmpty())
    return false;

  std::filesystem::path themePath =
      std::filesystem::path(g_sdkInitialPath) / name.toStdString();

  std::error_code ec;
  std::filesystem::remove_all(themePath, ec);
  if (ec) {
    qWarning() << "[SettingsBackendX11] Theme remove failed:"
               << QString::fromStdString(ec.message());
    return false;
  }

  reload();
  return true;
}
