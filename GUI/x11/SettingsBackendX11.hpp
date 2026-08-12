#pragma once
#include "SettingsBackend.hpp"

// X11 desktop build: there is no KWin plugin process and no polkit/KAuth
// helper installed. Theme install/remove is done as a plain filesystem copy
// under the user's own writable theme directory (no privilege escalation
// needed because we assume a per-user, not system-wide, theme location).
//
// ASSUMPTION: g_sdkInitialPath for this build points at a directory the
// current user can write to directly (e.g. ~/.local/share/ultralightwebcursor
// /themes). If your X11 packaging instead installs themes system-wide, this
// class will need a privilege-escalation path similar to the KWin variant.
class SettingsBackendX11 : public SettingsBackend {
  Q_OBJECT
public:
  explicit SettingsBackendX11(QObject *parent = nullptr);

  Q_INVOKABLE bool uploadTheme(const QString &path);
  Q_INVOKABLE bool removeTheme(const QString &name);
};
