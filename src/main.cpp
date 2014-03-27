// Standard headers
#include <iostream>
#include <string>

// Diesel headers
#ifdef __WIN__
#include "win32mmapplication.h"
#else
#include "gtkmmapplication.h"
#endif

int main(int argc, char** argv)
{
  std::cout<<"main"<<std::endl;

  int iResult = EXIT_SUCCESS;

  {
    diesel::cApplication application(argc, argv);

    iResult = application.Run();
  }

  return iResult;
}
