#ifndef DIESEL_APPLICATION_H
#define DIESEL_APPLICATION_H

// Spitfire headers
// Breathe headers
#include <breathe/util/cApplication.h>

// Diesel headers
#include "diesel.h"

namespace diesel
{
  class cApplication : public breathe::util::cApplication
  {
  public:
    cApplication(int argc, const char* const* argv);

  protected:
    int argc;
    const char* const* argv;

  private:
    virtual void _PrintHelp() const override;
    virtual string_t _GetVersion() const override;
    virtual bool _Run() override;
  };
}

#endif // DIESEL_APPLICATION_H
