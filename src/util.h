#ifndef DIESEL_UTIL_H
#define DIESEL_UTIL_H

// libvoodoomm headers
#include <libvoodoomm/cImage.h>

// Diesel headers
#include "diesel.h"

namespace diesel
{
  namespace util
  {
    bool GenerateThumbnail(voodoo::cImage& thumbnail, const voodoo::cImage& image, size_t width, size_t height);
  }
}

#endif // DIESEL_UTIL_H
