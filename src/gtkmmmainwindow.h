#ifndef gtkmmmainwindow_h
#define gtkmmmainwindow_h

// Gtkmm headers
#include <gtkmm.h>

// Spitfire headers
#include <spitfire/util/updatechecker.h>

// Diesel headers
#include "diesel.h"
#include "gtkmmopenglview.h"
#include "gtkmmscrollbar.h"
#include "settings.h"

namespace diesel
{
  class cGtkmmMainWindow : public Gtk::Window, public spitfire::util::cUpdateCheckerHandler
  {
  public:
    cGtkmmMainWindow(int argc, char** argv);

    void Run();

    void OnMenuFileQuit();

    void OnScrollBarScrolled(const cGtkmmScrollBar& widget);

  private:
    virtual bool on_key_press_event(GdkEventKey* event) override;

    bool event_box_button_press(GdkEventButton* pEvent);
    bool event_box_button_release(GdkEventButton* pEvent);
    bool event_box_scroll(GdkEventScroll* pEvent);
    bool event_box_motion_notify(GdkEventMotion* pEvent);

    virtual void OnNewVersionFound(int iMajorVersion, int iMinorVersion, const string_t& sDownloadPage) override;

    void OnMenuFileBrowseFiles();
    void OnMenuFileBrowseFolder();
    void OnMenuEditPreferences();
    void OnMenuHelpAbout();

    void OnActionBrowseFiles();
    void OnActionBrowseFolder();
    void OnActionAddFilesFromPicturesFolder();
    void OnActionStopLoading();

    cSettings settings;

    spitfire::util::cUpdateChecker updateChecker;

    // Menu and toolbar
    Glib::RefPtr<Gtk::UIManager> m_refUIManager;
    Glib::RefPtr<Gtk::ActionGroup> m_refActionGroup;

    // Layouts
    Gtk::VBox boxMainWindow;
    Gtk::Box boxToolbar;
    Gtk::HBox boxPositionSlider;
    Gtk::HBox boxCategoriesAndPlaylist;
    Gtk::HBox boxControlsAndToolbar;
    Gtk::VBox boxControls;
    Gtk::HBox boxStatusBar;

    // Controls
    Gtk::Button buttonAddFiles;
    Gtk::Button buttonAddFolder;

    // Status bar
    Gtk::Label statusBar;
    Gtk::Button buttonStopLoading;

    Gtk::EventBox eventBoxOpenglView;

    cGtkmmOpenGLView openglView;

    cGtkmmScrollBar scrollBar;
  };
}

#endif // gtkmmmainwindow_h
