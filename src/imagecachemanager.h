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

    #ifndef __WIN__
    static bool IsWineInstalled();
    #endif
    static bool IsUFRawBatchInstalled();
    static bool IsConvertInstalled();
    #ifdef __WIN__
    static bool IsAdobeDNGConverterInstalled();
    #endif

    static string_t GetOrCreateDNGForRawFile(const string_t& sRawFilePath);
    static string_t GetOrCreateThumbnailForDNGFile(const string_t& sDNGFilePath, IMAGE_SIZE imageSize);
    static string_t GetOrCreateThumbnailForImageFile(const string_t& sImageFilePath, IMAGE_SIZE imageSize);

  private:
    static string_t GetCacheFolderPath();
    #ifdef __WIN__
    static string_t GetUFRawBatchPath();
    static string_t GetConvertPath();
    #endif
    static string_t GetAdobeDNGConverterPath();
  };
}

#endif // DIESEL_IMAGECACHEMANAGER_H
