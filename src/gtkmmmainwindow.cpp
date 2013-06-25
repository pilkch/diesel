// Standard headers
#include <iostream>

// Gtk headers
#include <gdk/gdkkeysyms-compat.h>

// Gtkmm headers
#include <gtkmm/main.h>

// Spitfire headers
#include <spitfire/spitfire.h>

// Diesel headers
#include "gtkmmmainwindow.h"

namespace diesel
{
  cGtkmmMainWindow::cGtkmmMainWindow(int argc, char** argv) :
    openglView(*this)
  {
    set_title(BUILD_APPLICATION_NAME);
    set_default_size(800, 800);

    add(openglView);

    openglView.Init(argc,  argv);

    openglView.show();
  }

  void cGtkmmMainWindow::Run()
  {
    Gtk::Main::run(*this);
  }

  void cGtkmmMainWindow::OnMenuFileQuit()
  {
    //hide(); //Closes the main window to stop the Gtk::Main::run().
    Gtk::Main::quit();
  }

  bool cGtkmmMainWindow::on_key_press_event(GdkEventKey* event)
  {
    LOG<<"cGtkmmMainWindow::on_key_press_event"<<std::endl;

    // Send the event to the active widget
    if (openglView.is_focus()) return openglView.on_key_press_event(event);

    return false;
  }
}
