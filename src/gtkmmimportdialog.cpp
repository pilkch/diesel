// Spitfire headers
#include <spitfire/storage/filesystem.h>
#include <spitfire/util/datetime.h>

// libgtkmm headers
#include <libgtkmm/filebrowse.h>

// Diesel headers
#include "gtkmmimportdialog.h"

namespace diesel
{
  cGtkmmImportDialog::cGtkmmImportDialog(cSettings& _settings, Gtk::Window& parent) :
    Gtk::Dialog("Import", parent, true),
    settings(_settings),
    groupImport("Import"),
    importBrowseFromFolder("Browse..."),
    importBrowseToFolder("Browse..."),
    importSeparateFolderForEachYear("Separate folder for each year"),
    importSeparateFolderForEachDate("Separate folder for each date"),
    importDescription("Description"),
    groupAfterImport("After Import"),
    afterImportDeleteFromSourceFolderOnSuccessfulImport("Delete from source folder on successful import"),
    pOkButton(nullptr)
  {
    set_border_width(5);

    set_resizable();

    signal_response().connect(sigc::mem_fun(*this, &cGtkmmImportDialog::OnResponse));

    Gtk::Box* pBox = get_vbox();

    pBox->set_border_width(10);


    // Add the message label to the InfoBar
    Gtk::Container* pInfoBarContainer = static_cast<Gtk::Container*>(m_InfoBar.get_content_area());
    if (pInfoBarContainer != nullptr) pInfoBarContainer->add(m_InfoBarMessage_Label);

    // Add an ok button to the InfoBar:
    m_InfoBar.add_button(Gtk::Stock::OK, 0);

    pBox->pack_start(m_InfoBar, Gtk::PACK_SHRINK);


    pBox->pack_start(groupImport, Gtk::PACK_SHRINK);
    groupImport.add(boxImport);

    // Import
    Gtk::Label* pImportFromText = Gtk::manage(new Gtk::Label("Import From:"));
    boxImportFromFolder.pack_start(*pImportFromText, Gtk::PACK_SHRINK);
    boxImportFromFolder.pack_start(importFromFolder);
    boxImportFromFolder.pack_start(importBrowseFromFolder, Gtk::PACK_SHRINK);
    boxImport.pack_start(boxImportFromFolder, Gtk::PACK_SHRINK);

    Gtk::Label* pImportToText = Gtk::manage(new Gtk::Label("Import To:"));
    boxImportToFolder.pack_start(*pImportToText, Gtk::PACK_SHRINK);
    boxImportToFolder.pack_start(importToFolder);
    boxImportToFolder.pack_start(importBrowseToFolder, Gtk::PACK_SHRINK);
    boxImport.pack_start(boxImportToFolder, Gtk::PACK_SHRINK);

    boxImport.pack_start(importSeparateFolderForEachYear, Gtk::PACK_SHRINK);
    boxImport.pack_start(importSeparateFolderForEachDate, Gtk::PACK_SHRINK);
    boxImport.pack_start(importDescription, Gtk::PACK_SHRINK);
    boxImport.pack_start(importDescriptionText);
    boxImport.pack_start(importExampleFilePath, Gtk::PACK_SHRINK);

    // After Import
    pBox->pack_start(groupAfterImport, Gtk::PACK_SHRINK);

    groupAfterImport.add(boxAfterImport);

    boxAfterImport.pack_start(afterImportDeleteFromSourceFolderOnSuccessfulImport, Gtk::PACK_SHRINK);


    // Update the widget values
    importFromFolder.set_text(settings.GetLastImportFromFolder());
    importToFolder.set_text(settings.GetLastImportToFolder());
    importSeparateFolderForEachYear.set_active(settings.IsImportSeparateFolderForEachYear());
    importSeparateFolderForEachDate.set_active(settings.IsImportSeparateFolderForEachDate());
    importDescription.set_active(settings.IsImportDescription());
    importDescriptionText.set_text(settings.GetImportDescriptionText());
    afterImportDeleteFromSourceFolderOnSuccessfulImport.set_active(settings.IsAfterImportDeleteFromSourceFolderOnSuccessfulImport());


    UpdateExampleFilePath();


    // Install the event handlers
    m_InfoBar.signal_response().connect(sigc::mem_fun(*this, &cGtkmmImportDialog::OnInfoBarResponse));

    importFromFolder.signal_changed().connect(sigc::mem_fun(*this, &cGtkmmImportDialog::OnFromOrToChanged));
    importBrowseFromFolder.signal_clicked().connect(sigc::mem_fun(*this, &cGtkmmImportDialog::OnBrowseFromFolder));
    importToFolder.signal_changed().connect(sigc::mem_fun(*this, &cGtkmmImportDialog::OnFromOrToChanged));
    importBrowseToFolder.signal_clicked().connect(sigc::mem_fun(*this, &cGtkmmImportDialog::OnBrowseToFolder));
    importSeparateFolderForEachYear.signal_clicked().connect(sigc::mem_fun(*this, &cGtkmmImportDialog::OnControlChanged));
    importSeparateFolderForEachDate.signal_clicked().connect(sigc::mem_fun(*this, &cGtkmmImportDialog::OnControlChanged));
    importDescription.signal_clicked().connect(sigc::mem_fun(*this, &cGtkmmImportDialog::OnControlChanged));
    importDescriptionText.signal_changed().connect(sigc::mem_fun(*this, &cGtkmmImportDialog::OnControlChanged));


    // Add separator
    pBox->pack_start(separator, Gtk::PACK_SHRINK);

    // Add standard buttons
    add_button(Gtk::Stock::CANCEL, Gtk::ResponseType::RESPONSE_CANCEL);
    pOkButton = add_button(Gtk::Stock::OK, Gtk::ResponseType::RESPONSE_OK);
    pOkButton->set_label("Import");
    set_default_response(Gtk::ResponseType::RESPONSE_OK);

    // Make sure that our controls are in the correct state
    EnableControls();

    show_all_children();

    OnFromOrToChanged();
  }

  void cGtkmmImportDialog::OnFromOrToChanged()
  {
    LOG<<"cGtkmmImportDialog::OnFromOrToChanged"<<std::endl;

    if (!spitfire::filesystem::DirectoryExists(importFromFolder.get_text())) {
      m_InfoBarMessage_Label.set_text("The from folder does not exist");
      m_InfoBar.set_message_type(Gtk::MESSAGE_INFO);
      m_InfoBar.show();
      pOkButton->set_sensitive(false);
    } else if (!spitfire::filesystem::DirectoryExists(importToFolder.get_text())) {
      m_InfoBarMessage_Label.set_text("The to folder does not exist");
      m_InfoBar.set_message_type(Gtk::MESSAGE_INFO);
      m_InfoBar.show();
      pOkButton->set_sensitive(false);
    } else {
      // Clear the message and hide the info bar
      m_InfoBarMessage_Label.set_text("");
      m_InfoBar.hide();
      pOkButton->set_sensitive(true);
    }
  }

  void cGtkmmImportDialog::OnInfoBarResponse(int response)
  {
    // Clear the message and hide the info bar
    m_InfoBarMessage_Label.set_text("");
    m_InfoBar.hide();
  }

  void cGtkmmImportDialog::OnResponse(int response_id)
  {
    if (response_id == Gtk::ResponseType::RESPONSE_OK) {
      settings.SetLastImportFromFolder(importFromFolder.get_text());
      settings.SetLastImportToFolder(importToFolder.get_text());
      settings.SetImportSeparateFolderForEachYear(importSeparateFolderForEachYear.get_active());
      settings.SetImportSeparateFolderForEachDate(importSeparateFolderForEachDate.get_active());
      settings.SetImportDescription(importDescription.get_active());
      settings.SetImportDescriptionText(importDescriptionText.get_text());
      settings.SetAfterImportDeleteFromSourceFolderOnSuccessfulImport(afterImportDeleteFromSourceFolderOnSuccessfulImport.get_active());

      settings.Save();
    }
  }

  /*void cGtkmmImportDialog::OnActionImportClearFolders()
  {
    std::list<string_t> folders;
    settings.SetPreviousPhotoBrowserFolders(folders);
    settings.SetLastPhotoBrowserFolder("");
  }*/

  void cGtkmmImportDialog::EnableControls()
  {
    const bool bEnabled = importDescription.get_active();
    importDescriptionText.set_sensitive(bEnabled);
  }

  void cGtkmmImportDialog::UpdateExampleFilePath()
  {
    std::ostringstream o;

    o<<"Example: ";

    const string_t sImportToFolder = importToFolder.get_text();
    o<<sImportToFolder;
    if (sImportToFolder.back() != spitfire::filesystem::cFilePathSeparator) o<<spitfire::filesystem::sFilePathSeparator;
    spitfire::util::cDateTime now;
    if (importSeparateFolderForEachYear.get_active()) o<<now.GetYear()<<spitfire::filesystem::sFilePathSeparator;

    const bool bImportSeparateFolderForEachDate = importSeparateFolderForEachDate.get_active();

    string_t sDescription;
    if (importDescription.get_active()) sDescription = importDescriptionText.get_text();

    if (!sDescription.empty()) {
      if (bImportSeparateFolderForEachDate) o<<now.GetDateYYYYMMDD()<<" ";

      o<<sDescription<<spitfire::filesystem::sFilePathSeparator;
    } else if (bImportSeparateFolderForEachDate) o<<now.GetDateYYYYMMDD()<<spitfire::filesystem::sFilePathSeparator;

    importExampleFilePath.set_label(o.str());
  }

  void cGtkmmImportDialog::OnControlChanged()
  {
    UpdateExampleFilePath();
    EnableControls();
  }

  void cGtkmmImportDialog::OnBrowseFromFolder()
  {
    // Browse for the folder
    gtkmm::cGtkmmFolderDialog dialog;
    dialog.SetType(gtkmm::cGtkmmFolderDialog::TYPE::SELECT);
    dialog.SetCaption(TEXT("Select from folder"));
    dialog.SetDefaultFolder(importFromFolder.get_text());
    if (dialog.Run(*this)) {
      // Update the control
      importFromFolder.set_text(dialog.GetSelectedFolder());
    }
  }

  void cGtkmmImportDialog::OnBrowseToFolder()
  {
    // Browse for the folder
    gtkmm::cGtkmmFolderDialog dialog;
    dialog.SetType(gtkmm::cGtkmmFolderDialog::TYPE::SELECT);
    dialog.SetCaption(TEXT("Select to folder"));
    dialog.SetDefaultFolder(importToFolder.get_text());
    if (dialog.Run(*this)) {
      // Update the control
      importToFolder.set_text(dialog.GetSelectedFolder());

      // Update the example file path
      UpdateExampleFilePath();
    }
  }

  bool cGtkmmImportDialog::Run()
  {
    return (run() == Gtk::ResponseType::RESPONSE_OK);
  }
}
