#include "SettingsBackendWindows.hpp"
#include <QDir>
#include <QDebug>
#include <filesystem>

using namespace UltralightWebCursorM;

SettingsBackendWindows::SettingsBackendWindows(QObject *parent)
    : SettingsBackend(parent) {}

bool SettingsBackendWindows::uploadTheme(const QString &path) {
  QDir srcDir(QDir::cleanPath(path));
  if (!srcDir.exists()) {
    qWarning() << "[SettingsBackendWindows] Theme folder not found:" << path;
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
    qWarning() << "[SettingsBackendWindows] Theme copy failed:"
               << QString::fromStdString(ec.message());
    return false;
  }

  reload();
  return true;
}

bool SettingsBackendWindows::removeTheme(const QString &name) {
  if (name.isEmpty())
    return false;

  std::filesystem::path themePath =
      std::filesystem::path(g_sdkInitialPath) / name.toStdString();

  std::error_code ec;
  std::filesystem::remove_all(themePath, ec);
  if (ec) {
    qWarning() << "[SettingsBackendWindows] Theme remove failed:"
               << QString::fromStdString(ec.message());
    return false;
  }

  reload();
  return true;
}
