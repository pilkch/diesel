#ifndef DIESEL_WIN32MMSETTINGSDIALOG_H
#define DIESEL_WIN32MMSETTINGSDIALOG_H

// libwin32mm headers
#include <libwin32mm/window.h>

// Diesel headers
#include "settings.h"

namespace diesel
{
  bool OpenSettingsDialog(cSettings& settings, win32mm::cWindow& parent);
}

#endif // DIESEL_WIN32MMSETTINGSDIALOG_H
