// Standard headers
#include <cstdlib>
#include <iostream>

// Spitfire headers
#include <spitfire/algorithm/md5.h>
#include <spitfire/storage/filesystem.h>
#include <spitfire/util/log.h>

// Diesel headers
#include "imagecachemanager.h"

namespace diesel
{
  void cImageCacheManager::ClearCache()
  {
    const string_t sFolderPath = GetCacheFolderPath();
    if (spitfire::filesystem::DirectoryExists(sFolderPath)) spitfire::filesystem::DeleteDirectory(sFolderPath);
  }

  string_t cImageCacheManager::GetCacheFolderPath()
  {
    // Create the directory
    const string_t sFolder = spitfire::filesystem::GetThisApplicationSettingsDirectory();
    spitfire::filesystem::CreateDirectory(sFolder);

    return sFolder + TEXT("cache");
  }
}
