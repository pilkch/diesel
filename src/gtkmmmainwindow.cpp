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
    updateChecker(*this),
    openglView(*this)
  {
    set_title(BUILD_APPLICATION_NAME);
    set_default_size(800, 800);

    add(openglView);

    openglView.Init(argc,  argv);

    openglView.show();

    // Start the update checker now that we have finished doing the serious work
    updateChecker.Run();
  }

  void cGtkmmMainWindow::Run()
  {
    Gtk::Main::run(*this);
  }

  void cGtkmmMainWindow::OnMenuFileQuit()
  {
    // Tell the update checker thread to stop soon
    if (updateChecker.IsRunning()) updateChecker.StopThreadSoon();

    // Tell the update checker thread to stop now
    if (updateChecker.IsRunning()) updateChecker.StopThreadNow();

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

  void cGtkmmMainWindow::OnNewVersionFound(int iMajorVersion, int iMinorVersion, const string_t& sDownloadPage)
  {
    LOG<<"cGtkmmMainWindow::OnNewVersionFound "<<iMajorVersion<<"."<<iMinorVersion<<", "<<sDownloadPage<<std::endl;
  }
}
