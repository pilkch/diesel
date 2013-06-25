#ifndef gtkmmmainwindow_h
#define gtkmmmainwindow_h

// Gtkmm headers
#include <gtkmm/window.h>

// Diesel headers
#include "gtkmmopenglview.h"

namespace diesel
{
  class cGtkmmMainWindow : public Gtk::Window
  {
  public:
    cGtkmmMainWindow(int argc, char** argv);

    void Run();

    void OnMenuFileQuit();

  private:
    virtual bool on_key_press_event(GdkEventKey* event) override;

    cGtkmmOpenGLView openglView;
  };
}

#endif // gtkmmmainwindow_h
