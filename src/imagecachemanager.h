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

    static bool IsWineInstalled();
    static bool IsUFRawBatchInstalled();
    static bool IsConvertInstalled();

    string_t GetOrCreateDNGForRawFile(const string_t& sRawFilePath);
    string_t GetOrCreateThumbnailForDNGFile(const string_t& sDNGFilePath, IMAGE_SIZE imageSize);
    string_t GetOrCreateThumbnailForImageFile(const string_t& sImageFilePath, IMAGE_SIZE imageSize);

  private:
    static string_t GetCacheFolderPath();
  };
}

#endif // DIESEL_IMAGECACHEMANAGER_H
