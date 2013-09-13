#ifndef DIESEL_GTKMMIMPORTDIALOG_H
#define DIESEL_GTKMMIMPORTDIALOG_H

// Gtkmm headers
#include <gtkmm.h>

// Diesel headers
#include "diesel.h"
#include "settings.h"

namespace diesel
{
  class cGtkmmImportDialog : public Gtk::Dialog
  {
  public:
    cGtkmmImportDialog(cSettings& settings, Gtk::Window& parent);

    bool Run();

  private:
    void EnableControls();
    void UpdateExampleFilePath();

    void OnControlChanged();
    void OnFromOrToChanged();

    void OnBrowseFromFolder();
    void OnBrowseToFolder();

    void OnInfoBarResponse(int response);

    void OnResponse(int response_id);

    cSettings& settings;

    Gtk::Alignment m_Alignment;

    // Controls
    Gtk::InfoBar m_InfoBar;
    Gtk::Label m_InfoBarMessage_Label;

    // Import
    Gtk::Frame groupImport;
    Gtk::VBox boxImport;
    Gtk::HBox boxImportFromFolder;
    Gtk::Entry importFromFolder;
    Gtk::Button importBrowseFromFolder;
    Gtk::HBox boxImportToFolder;
    Gtk::Entry importToFolder;
    Gtk::Button importBrowseToFolder;
    Gtk::CheckButton importSeparateFolderForEachYear;
    Gtk::CheckButton importSeparateFolderForEachDate;
    Gtk::CheckButton importDescription;
    Gtk::Entry importDescriptionText;
    Gtk::Label importExampleFilePath;
    // After Import
    Gtk::Frame groupAfterImport;
    Gtk::VBox boxAfterImport;
    Gtk::CheckButton afterImportDeleteFromSourceFolderOnSuccessfulImport;

    Gtk::Separator separator;

    Gtk::Button* pOkButton;
  };
}

#endif // DIESEL_GTKMMIMPORTDIALOG_H
