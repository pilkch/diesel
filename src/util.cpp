// Diesel headers
#include "util.h"

namespace diesel
{
  namespace util
  {
    bool GenerateThumbnail(voodoo::cImage& thumbnail, const voodoo::cImage& image, size_t width, size_t height)
    {
      // TODO: Actually generate a thumbnail
      thumbnail = image;

      return true;
    }
  }
}
