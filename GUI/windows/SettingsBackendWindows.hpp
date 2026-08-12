#pragma once
#include "SettingsBackend.hpp"

// Windows desktop build: no polkit/KAuth equivalent is used here. Themes are
// installed under a per-user AppData directory, which requires no elevation.
//
// ASSUMPTION: g_sdkInitialPath for this build resolves to something like
// %LOCALAPPDATA%/UltralightWebCursor/themes. If your Windows installer
// instead places themes under Program Files (machine-wide), copying there
// will require a UAC elevation prompt, which is not implemented here.
class SettingsBackendWindows : public SettingsBackend {
  Q_OBJECT
public:
  explicit SettingsBackendWindows(QObject *parent = nullptr);

  Q_INVOKABLE bool uploadTheme(const QString &path);
  Q_INVOKABLE bool removeTheme(const QString &name);
};
