#ifndef DIESEL_IMAGECACHEMANAGER_H
#define DIESEL_IMAGECACHEMANAGER_H

// libvoodoomm headers
#include <libvoodoomm/cImage.h>

// Diesel headers
#include "diesel.h"

namespace diesel
{
  class cImageCacheManager
  {
  public:
    static void ClearCache();

  private:
    static string_t GetCacheFolderPath();
  };
}

#endif // DIESEL_IMAGECACHEMANAGER_H
