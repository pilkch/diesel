#ifndef DIESEL_GTKMMPREFERENCESDIALOG_H
#define DIESEL_GTKMMPREFERENCESDIALOG_H

// Standard headers
#include <vector>

// Gtkmm headers
#include <gtkmm.h>

// Diesel headers
#include "diesel.h"
#include "settings.h"

namespace diesel
{
  class cGtkmmPreferencesDialog : public Gtk::Dialog
  {
  public:
    cGtkmmPreferencesDialog(cSettings& settings, Gtk::Window& parent);

    bool Run();

  private:
    void OnResponse(int response_id);
    void OnEnableControls();

    void OnActionHistoryClearFolders();
    void OnActionHistoryClearCache();

    cSettings& settings;

    Gtk::Alignment m_Alignment;

    // Controls
    // History
    Gtk::Frame groupHistory;
    Gtk::VBox boxHistory;
    Gtk::Button historyClearFolders;
    Gtk::Button historyClearCache;

    Gtk::Separator separator;

    Gtk::Button* pOkButton;
  };
}

#endif // DIESEL_GTKMMPREFERENCESDIALOG_H
