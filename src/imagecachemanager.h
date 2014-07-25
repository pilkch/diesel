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
    static void EnforceMaximumCacheSize(size_t nMaximumCacheSizeGB);
    static void ClearCache();

    static bool IsWineInstalled();
    static bool IsUFRawBatchInstalled();
    static bool IsConvertInstalled();

    static string_t GetOrCreateDNGForRawFile(const string_t& sRawFilePath);
    static string_t GetOrCreateThumbnailForDNGFile(const string_t& sDNGFilePath, IMAGE_SIZE imageSize);
    static string_t GetOrCreateThumbnailForImageFile(const string_t& sImageFilePath, IMAGE_SIZE imageSize);

  private:
    static string_t GetCacheFolderPath();
    static string_t GetAdobeDNGConverterPath();
  };
}

#endif // DIESEL_IMAGECACHEMANAGER_H
