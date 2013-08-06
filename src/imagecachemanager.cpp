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
  void cImageCacheManager::EnforceMaximumCacheSize(size_t nMaximumCacheSizeGB)
  {
    //const string_t sCacheFolderPath = GetCacheFolderPath();
    //if (spitfire::filesystem::DirectoryExists(sCacheFolderPath)) spitfire::filesystem::DeleteDirectory(sCacheFolderPath);

    // TODO: Get a list of all files in the folder with sizes sorted by date modified, remove the last ones until we remove x MB down to nMaximumSizeGB * 1024;
  }

  void cImageCacheManager::ClearCache()
  {
    const string_t sCacheFolderPath = GetCacheFolderPath();
    if (spitfire::filesystem::DirectoryExists(sCacheFolderPath)) spitfire::filesystem::DeleteDirectory(sCacheFolderPath);
  }

  string_t cImageCacheManager::GetCacheFolderPath()
  {
    const string_t sFolder = spitfire::filesystem::GetThisApplicationSettingsDirectory();
    spitfire::filesystem::CreateDirectory(sFolder);
    ASSERT(spitfire::filesystem::DirectoryExists(sFolder));

    const string_t sCacheFolder = spitfire::filesystem::MakeFilePath(sFolder, TEXT("cache"));
    spitfire::filesystem::CreateDirectory(sCacheFolder);
    ASSERT(spitfire::filesystem::DirectoryExists(sCacheFolder));

    return sCacheFolder;
  }

  bool cImageCacheManager::IsWineInstalled()
  {
    const int iResult = system("which wine");
    return (iResult == 0);
  }

  bool cImageCacheManager::IsUFRawBatchInstalled()
  {
    const int iResult = system("which ufraw-batch");
    return (iResult == 0);
  }

  bool cImageCacheManager::IsConvertInstalled()
  {
    const int iResult = system("which convert");
    return (iResult == 0);
  }

  string_t cImageCacheManager::GetOrCreateDNGForRawFile(const string_t& sRawFilePath)
  {
    LOG<<"cImageCacheManager::GetOrCreateDNGForRawFile \""<<sRawFilePath<<"\""<<std::endl;

    ASSERT(IsWineInstalled());

    // TODO: Use the actual dng sdk like this instead?
    // https://projects.kde.org/projects/extragear/graphics/kipi-plugins/repository/revisions/master/show/dngconverter

    std::ostringstream o;
    o<<"wine \"C:\\Program Files (x86)\\Adobe\\Adobe DNG Converter.exe\" -c \""<<sRawFilePath<<"\"";
    #ifndef BUILD_DEBUG
    o<<" &> /dev/null";
    #endif
    const int iResult = system(o.str().c_str());
    if (iResult != 0) {
      LOG<<"cImageCacheManager::GetOrCreateDNGForRawFile wine returned "<<iResult<<", returning \"\""<<std::endl;
      return "";
    }

    const string_t sFolder = spitfire::filesystem::GetFolder(sRawFilePath);
    const string_t sFile = spitfire::filesystem::GetFileNoExtension(sRawFilePath);
    const string_t sDNGFilePath = spitfire::filesystem::MakeFilePath(sFolder, sFile + ".dng");
    return sDNGFilePath;
  }

  string_t cImageCacheManager::GetOrCreateThumbnailForDNGFile(const string_t& sDNGFilePath, IMAGE_SIZE imageSize)
  {
    LOG<<"cImageCacheManager::GetOrCreateThumbnailForDNGFile \""<<sDNGFilePath<<"\""<<std::endl;

    ASSERT(IsUFRawBatchInstalled());

    spitfire::algorithm::cMD5 md5;
    md5.CalculateForFile(sDNGFilePath);

    const string_t sCacheFolder = GetCacheFolderPath();

    string_t sFileJPG = "full.jpg";
    size_t size = 0;

    switch (imageSize) {
      case IMAGE_SIZE::THUMBNAIL: {
        sFileJPG = "thumbnail.jpg";
        size = 200;
        break;
      }
    }

    const string_t sFilePathJPG = spitfire::filesystem::MakeFilePath(sCacheFolder, md5.GetResultFormatted() + "_" + sFileJPG);
    if (spitfire::filesystem::FileExists(sFilePathJPG)) return sFilePathJPG;

    const string_t sFolderJPG = spitfire::filesystem::GetFolder(sFilePathJPG);

    std::ostringstream o;
    o<<"ufraw-batch --out-type=jpg";
    if (size != 0) o<<" --embedded-image --size="<<size;
    o<<" \""<<sDNGFilePath<<"\" --overwrite --out-path=\""<<sFolderJPG<<"\"";
    #ifndef BUILD_DEBUG
    o<<" --silent";
    #endif
    const int iResult = system(o.str().c_str());
    if (iResult != 0) {
      LOG<<"cImageCacheManager::GetOrCreateThumbnailForDNGFile ufraw-batch returned "<<iResult<<", returning \"\""<<std::endl;
      return "";
    }

    // ufraw-batch doesn't respect the output folder if we provide our own output filename, so we have to rename the file after it is converted
    const string_t sNameJPG = spitfire::filesystem::GetFileNoExtension(sFilePathJPG);

    // Try to rename from file.embedded.jpg to abcdefghi_file.jpg
    const string_t sFileUFRawEmbeddedJPG = spitfire::filesystem::GetFileNoExtension(sDNGFilePath) + ".embedded.jpg";
    string_t sFilePathUFRawJPG = spitfire::filesystem::MakeFilePath(sFolderJPG, sFileUFRawEmbeddedJPG);
    LOG<<"cImageCacheManager::GetOrCreateThumbnailForDNGFile Looking for \""<<sFilePathUFRawJPG<<"\""<<std::endl;
    if (spitfire::filesystem::FileExists(sFilePathUFRawJPG)) {
      if (!spitfire::filesystem::MoveFile(sFilePathUFRawJPG, sFilePathJPG)) {
        LOG<<"cImageCacheManager::GetOrCreateThumbnailForDNGFile Failed to move the file from \""<<sFolderJPG<<"\" to \""<<sFilePathJPG<<"\", returning \"\""<<std::endl;
        return "";
      }
    } else {
      // Try to rename from file.jpg to abcdefghi_file.jpg
      const string_t sFileUFRawJPG = spitfire::filesystem::GetFileNoExtension(sDNGFilePath) + ".jpg";
      string_t sFilePathUFRawJPG = spitfire::filesystem::MakeFilePath(sFolderJPG, sFileUFRawJPG);
      LOG<<"cImageCacheManager::GetOrCreateThumbnailForDNGFile Looking for \""<<sFilePathUFRawJPG<<"\""<<std::endl;
      if (spitfire::filesystem::FileExists(sFilePathUFRawJPG) && !spitfire::filesystem::MoveFile(sFilePathUFRawJPG, sFilePathJPG)) {
        LOG<<"cImageCacheManager::GetOrCreateThumbnailForDNGFile Failed to move the file from \""<<sFolderJPG<<"\" to \""<<sFilePathJPG<<"\", returning \"\""<<std::endl;
        return "";
      }
    }

    if (!spitfire::filesystem::FileExists(sFilePathJPG)) {
      LOG<<"cImageCacheManager::GetOrCreateThumbnailForDNGFile Failed to create the thumbnail image \""<<sFilePathJPG<<"\", returning \"\""<<std::endl;
      return "";
    }

    return sFilePathJPG;
  }

  string_t cImageCacheManager::GetOrCreateThumbnailForImageFile(const string_t& sImageFilePath, IMAGE_SIZE imageSize)
  {
    LOG<<"cImageCacheManager::GetOrCreateThumbnailForImageFile \""<<sImageFilePath<<"\""<<std::endl;

    ASSERT(IsConvertInstalled());

    spitfire::algorithm::cMD5 md5;
    md5.CalculateForFile(sImageFilePath);

    const string_t sCacheFolder = GetCacheFolderPath();

    string_t sFileJPG = "full.jpg";
    size_t width = 0;
    size_t height = 0;

    switch (imageSize) {
      case IMAGE_SIZE::THUMBNAIL: {
        sFileJPG = "thumbnail.jpg";
        width = 200;
        height = 200;
        break;
      }
    }

    const string_t sFilePathJPG = spitfire::filesystem::MakeFilePath(sCacheFolder, md5.GetResultFormatted() + "_" + sFileJPG);
    if (spitfire::filesystem::FileExists(sFilePathJPG)) return sFilePathJPG;

    const string_t sFolderJPG = spitfire::filesystem::GetFolder(sFilePathJPG);

    std::ostringstream o;
    o<<"convert "<<sImageFilePath;
    if ((width != 0) && (height != 0)) o<<" -resize "<<width<<"x"<<height;
    o<<" -auto-orient "<<sFilePathJPG;
    const int iResult = system(o.str().c_str());
    if (iResult != 0) {
      LOG<<"cImageCacheManager::GetOrCreateThumbnailForImageFile convert returned "<<iResult<<", returning \"\""<<std::endl;
      return "";
    }

    if (!spitfire::filesystem::FileExists(sFilePathJPG)) {
      LOG<<"cImageCacheManager::GetOrCreateThumbnailForImageFile Failed to create the thumbnail image \""<<sFilePathJPG<<"\", returning \"\""<<std::endl;
      return "";
    }

    return sFilePathJPG;
  }
}
