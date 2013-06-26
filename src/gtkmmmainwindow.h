#ifndef gtkmmmainwindow_h
#define gtkmmmainwindow_h

// Gtkmm headers
#include <gtkmm/window.h>

// Spitfire headers
#include <spitfire/util/updatechecker.h>

// Diesel headers
#include "diesel.h"
#include "gtkmmopenglview.h"

namespace diesel
{
  class cGtkmmMainWindow : public Gtk::Window, public spitfire::util::cUpdateCheckerHandler
  {
  public:
    cGtkmmMainWindow(int argc, char** argv);

    void Run();

    void OnMenuFileQuit();

  private:
    virtual bool on_key_press_event(GdkEventKey* event) override;

    virtual void OnNewVersionFound(int iMajorVersion, int iMinorVersion, const string_t& sDownloadPage) override;

    spitfire::util::cUpdateChecker updateChecker;

    cGtkmmOpenGLView openglView;
  };
}

#endif // gtkmmmainwindow_h
