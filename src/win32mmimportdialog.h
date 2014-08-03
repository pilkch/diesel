#ifndef DIESEL_WIN32MMIMPORTDIALOG_H
#define DIESEL_WIN32MMIMPORTDIALOG_H

// libwin32mm headers
#include <libwin32mm/window.h>

// Diesel headers
#include "settings.h"

namespace diesel
{
  bool OpenImportDialog(cSettings& settings, win32mm::cWindow& parent);
}

#endif // DIESEL_WIN32MMIMPORTDIALOG_H
