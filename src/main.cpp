// Standard headers
#include <iostream>
#include <string>

// Gtkmm headers
#include <gtkmm/main.h>

// Spitfire headers
#include <spitfire/util/string.h>
#include <spitfire/util/thread.h>

// Diesel headers
#include "gtkmmmainwindow.h"

int main(int argc, char* argv[])
{
  std::cout<<"main"<<std::endl;

  #if defined(BUILD_DEBUG) && defined(PLATFORM_LINUX_OR_UNIX)
  // KDevelop only shows output sent to cerr, we redirect cout to cerr here so that it will show up
  // Back up cout's streambuf
  std::streambuf* backup = std::cout.rdbuf();

  // Assign cerr's streambuf to cout
  std::cout.rdbuf(std::cerr.rdbuf());
  #endif

  bool bIsSuccess = false;

  {
    spitfire::util::SetMainThread();

    spitfire::string::Init();

    Gtk::Main kit(argc, argv);
  
    diesel::cGtkmmMainWindow window(argc, argv);
    window.Run();
  }

  #if defined(BUILD_DEBUG) && defined(PLATFORM_LINUX_OR_UNIX)
  // Restore the original cout streambuf
  std::cout.rdbuf(backup);
  #endif

  return bIsSuccess ? EXIT_SUCCESS : EXIT_FAILURE;
}
