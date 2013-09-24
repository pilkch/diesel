#ifndef gtkmmmainwindow_h
#define gtkmmmainwindow_h

// Gtkmm headers
#include <gtkmm.h>

// libgtkmm headers
#include <libgtkmm/dispatcher.h>
#include <libgtkmm/icontheme.h>

// Spitfire headers
#include <spitfire/util/updatechecker.h>

// Diesel headers
#include "diesel.h"
#include "gtkmmphotobrowser.h"
#include "settings.h"

namespace diesel
{
  class cGtkmmMainWindowEventNewVersionFound
  {
  public:
    cGtkmmMainWindowEventNewVersionFound(int iMajorVersion, int iMinorVersion, const string_t& sDownloadPage);

    void EventFunction(cGtkmmMainWindow& mainWindow);

  private:
    int iMajorVersion;
    int iMinorVersion;
    string_t sDownloadPage;
  };

  class cGtkmmMainWindow : public Gtk::Window, public spitfire::util::cUpdateCheckerHandler
  {
  public:
    friend class gtkmm::cIconTheme;
    friend class cGtkmmOpenGLView;
    friend class cGtkmmScrollBar;
    friend class cGtkmmPhotoBrowser;
    friend class cGtkmmMainWindowEventNewVersionFound;

    cGtkmmMainWindow(int argc, char** argv);

    void Run();

  protected:
    void OnThemeChanged();

    void OnMenuFileQuit();

    void OnPhotoBrowserRightClick();

    void OnPhotoBrowserChangedFolder(const string_t& sFolderPath);
    void OnPhotoBrowserLoadedFileOrFolder();
    void OnPhotoBrowserFileFound();
    void OnPhotoBrowserLoadedFilesClear();
    void OnPhotoBrowserSinglePhotoMode(const string_t& sFileNameNoExtension);
    void OnPhotoBrowserPhotoCollageMode();
    void OnPhotoBrowserSelectionChanged();

  private:
    virtual void OnNewVersionFound(int iMajorVersion, int iMinorVersion, const string_t& sDownloadPage) override;

    virtual bool on_delete_event(GdkEventAny* pEvent);

    void DestroyCommon();

    void OnMenuFileBrowseFiles();
    void OnMenuFileBrowseFolder();
    void OnMenuFileImportFolder();
    void OnMenuEditPreferences();
    void OnMenuHelpAbout();

    void OnActionChangeFolder();
    void OnActionFolderUp();
    void OnActionFolderShowInFileManager();
    void OnActionBrowseFiles();
    void OnActionBrowseFolder();
    void OnActionAddFilesFromPicturesFolder();
    void OnActionStopLoading();
    void OnActionRemovePhoto();

    void ApplySettings();

    void ChangeFolder(const string_t& sFolder);

    void UpdateIcons();
    void UpdateStatusBar();

    cSettings settings;

    spitfire::util::cUpdateChecker updateChecker;

    gtkmm::cGtkmmRunOnMainThread<cGtkmmMainWindow, cGtkmmMainWindowEventNewVersionFound> notifyMainThread;

    std::list<string_t> previousFolders;

    // Menu and toolbar
    Glib::RefPtr<Gtk::UIManager> m_refUIManager;
    Glib::RefPtr<Gtk::ActionGroup> m_refActionGroup;

    // Photo browser right click menu
    Glib::RefPtr<Gtk::UIManager> popupUIManagerRef;
    Gtk::Menu* pMenuPopup;

    // Layouts
    Gtk::VBox boxMainWindow;
    Gtk::VBox boxToolbar;
    Gtk::HBox boxPositionSlider;
    Gtk::HBox boxControlsAndToolbar;
    Gtk::VBox boxControls;
    Gtk::HBox boxStatusBar;

    // Controls
    Gtk::ComboBoxText comboBoxFolder;
    Gtk::Button buttonFolderUp;
    Gtk::Button buttonFolderShowInFileManager;
    Gtk::Button buttonAddFiles;
    Gtk::Button buttonAddFolder;

    // Status bar
    Gtk::Label statusBar;
    Gtk::Button buttonStopLoading;

    cGtkmmPhotoBrowser photoBrowser;

    gtkmm::cIconTheme iconTheme;
  };
}

#endif // gtkmmmainwindow_h
