// Standard headers
#include <iostream>
#include <string>

// Diesel headers
#include "application.h"

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
