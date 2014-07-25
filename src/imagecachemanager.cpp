// Standard headers
#include <cstdlib>
#include <iostream>

// Spitfire headers
#include <spitfire/algorithm/md5.h>
#include <spitfire/storage/filesystem.h>
#include <spitfire/util/datetime.h>
#include <spitfire/util/log.h>

// Diesel headers
#include "imagecachemanager.h"

#ifdef UNICODE
int system(const wchar_t* szCommand)
{
  return _wsystem(szCommand);
}
#endif

namespace diesel
{
  class cFileAgeAndSize
  {
  public:
    cFileAgeAndSize(const string_t& sFilePath, size_t nFileSizeBytes);

    string_t sFilePath;
    size_t nFileSizeBytes;
  };

  cFileAgeAndSize::cFileAgeAndSize(const string_t& _sFilePath, size_t _nFileSizeBytes) :
    sFilePath(_sFilePath),
    nFileSizeBytes(_nFileSizeBytes)
  {
  }

  void cImageCacheManager::EnforceMaximumCacheSize(size_t nMaximumCacheSizeGB)
  {
    const string_t sCacheFolderPath = GetCacheFolderPath();

    size_t nFolderSizeBytes = 0;

    // Get a list of all the files in the cache folder sorted by
    std::multimap<spitfire::util::cDateTime, cFileAgeAndSize*> files;
    for (spitfire::filesystem::cFolderIterator iter(sCacheFolderPath); iter.IsValid(); iter.Next()) {
      ASSERT(!iter.IsFolder());
      const string_t sFilePath = iter.GetFullPath();
      const spitfire::util::cDateTime dateTimeModified = spitfire::filesystem::GetLastModifiedDate(sFilePath);
      const size_t nFileSizeBytes = spitfire::filesystem::GetFileSizeBytes(sFilePath);

      files.insert(std::make_pair(dateTimeModified, new cFileAgeAndSize(sFilePath, nFileSizeBytes)));

      nFolderSizeBytes += nFileSizeBytes;
    }

    const size_t nMaximumCacheSizeBytes = nMaximumCacheSizeGB * 1024 * 1024 * 1024;
    if (nFolderSizeBytes > nMaximumCacheSizeBytes) {
      // Remove the last files until we have removed enough files to get below nMaximumCacheSizeGB * 1024
      const size_t nDifferenceBytes = nFolderSizeBytes - nMaximumCacheSizeBytes;
      size_t nDeletedBytes = 0;
      std::multimap<spitfire::util::cDateTime, cFileAgeAndSize*>::const_iterator iter = files.begin();
      const std::multimap<spitfire::util::cDateTime, cFileAgeAndSize*>::const_iterator iterEnd = files.end();
      while (iter != iterEnd) {
        cFileAgeAndSize* pEntry = iter->second;
        spitfire::filesystem::DeleteFile(pEntry->sFilePath);

        // Break if we have now deleted enough bytes
        nDeletedBytes += pEntry->nFileSizeBytes;
        if (nDeletedBytes >= nDifferenceBytes) break;

        iter++;
      }
    }

    // Delete the items
    std::multimap<spitfire::util::cDateTime, cFileAgeAndSize*>::const_iterator iter = files.begin();
    const std::multimap<spitfire::util::cDateTime, cFileAgeAndSize*>::const_iterator iterEnd = files.end();
    while (iter != iterEnd) {
      cFileAgeAndSize* pEntry = iter->second;
      spitfire::SAFE_DELETE(pEntry);

      iter++;
    }
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
    LOG<<"cImageCacheManager::IsWineInstalled returned "<<iResult<<std::endl;
    return (iResult == 0);
  }

  bool cImageCacheManager::IsUFRawBatchInstalled()
  {
    const int iResult = system("which ufraw-batch");
    LOG<<"cImageCacheManager::IsUFRawBatchInstalled returned "<<iResult<<std::endl;
    return (iResult == 0);
  }

  bool cImageCacheManager::IsConvertInstalled()
  {
    const int iResult = system("which convert");
    LOG<<"cImageCacheManager::IsConvertInstalled returned "<<iResult<<std::endl;
    return (iResult == 0);
  }

  string_t cImageCacheManager::GetOrCreateDNGForRawFile(const string_t& sRawFilePath)
  {
    LOG<<"cImageCacheManager::GetOrCreateDNGForRawFile \""<<sRawFilePath<<"\""<<std::endl;

    ASSERT(spitfire::filesystem::FileExists(sRawFilePath));

    if (!IsWineInstalled()) {
      LOG<<"cImageCacheManager::GetOrCreateDNGForRawFile wine is not installed, returning \"\""<<std::endl;
      return TEXT("");
    }

    // TODO: Use the actual dng sdk like this instead?
    // https://projects.kde.org/projects/extragear/graphics/kipi-plugins/repository/revisions/master/show/dngconverter
    const string_t sFolder = spitfire::filesystem::GetFolder(sRawFilePath);
    const string_t sFile = spitfire::filesystem::GetFileNoExtension(sRawFilePath);
    const string_t sDNGFilePath = spitfire::filesystem::MakeFilePath(sFolder, sFile + TEXT(".dng"));

    ostringstream_t o;
    o<<"wine \"C:\\Program Files (x86)\\Adobe\\Adobe DNG Converter.exe\" -c \""<<sRawFilePath<<"\"";
    #ifndef BUILD_DEBUG
    o<<" &> /dev/null";
    #endif
    const string_t sCommandLine = o.str();
    const int iResult = system(sCommandLine.c_str());
    if (iResult != 0) {
      LOG<<"cImageCacheManager::GetOrCreateDNGForRawFile wine returned "<<iResult<<" for \""<<sCommandLine<<"\", returning \"\""<<std::endl;
      return TEXT("");
    }

    return sDNGFilePath;
  }

  string_t cImageCacheManager::GetOrCreateThumbnailForDNGFile(const string_t& sDNGFilePath, IMAGE_SIZE imageSize)
  {
    LOG<<"cImageCacheManager::GetOrCreateThumbnailForDNGFile \""<<sDNGFilePath<<"\""<<std::endl;

    ASSERT(spitfire::filesystem::FileExists(sDNGFilePath));

    if (!IsUFRawBatchInstalled()) {
      LOG<<"cImageCacheManager::GetOrCreateThumbnailForDNGFile ufraw-batch is not installed, returning \"\""<<std::endl;
      return TEXT("");
    }

    spitfire::algorithm::cMD5 md5;
    md5.CalculateForFile(sDNGFilePath);

    const string_t sCacheFolder = GetCacheFolderPath();

    string_t sFileJPG = TEXT("full.jpg");
    size_t size = 0;

    switch (imageSize) {
      case IMAGE_SIZE::THUMBNAIL: {
        sFileJPG = TEXT("thumbnail.jpg");
        size = 200;
        break;
      }
    }

    const string_t sFilePathJPG = spitfire::filesystem::MakeFilePath(sCacheFolder, md5.GetResultFormatted() + TEXT("_") + sFileJPG);
    if (spitfire::filesystem::FileExists(sFilePathJPG)) return sFilePathJPG;

    const string_t sFolderJPG = spitfire::filesystem::GetFolder(sFilePathJPG);

    const string_t sFileUFRawEmbeddedJPG = spitfire::filesystem::GetFileNoExtension(sDNGFilePath) + TEXT(".embedded.jpg");
    string_t sFilePathUFRawJPG = spitfire::filesystem::MakeFilePath(sFolderJPG, sFileUFRawEmbeddedJPG);

    ostringstream_t o;
    o<<"ufraw-batch --out-type=jpg";
    if (size != 0) o<<" --embedded-image --size="<<size;
    o<<" \""<<sDNGFilePath<<"\" --overwrite --out-path=\""<<sFolderJPG<<"\"";
    #ifndef BUILD_DEBUG
    o<<" --silent";
    #endif
    const string_t sCommandLine = o.str();
    const int iResult = system(sCommandLine.c_str());
    if (iResult != 0) {
      LOG<<"cImageCacheManager::GetOrCreateThumbnailForDNGFile ufraw-batch returned "<<iResult<<" for \""<<sCommandLine<<"\", returning \"\""<<std::endl;
      return TEXT("");
    }

    // ufraw-batch doesn't respect the output folder if we provide our own output filename, so we have to rename the file after it is converted
    const string_t sNameJPG = spitfire::filesystem::GetFileNoExtension(sFilePathJPG);

    // Try to rename from file.embedded.jpg to abcdefghi_file.jpg
    LOG<<"cImageCacheManager::GetOrCreateThumbnailForDNGFile Looking for \""<<sFilePathUFRawJPG<<"\""<<std::endl;
    if (spitfire::filesystem::FileExists(sFilePathUFRawJPG)) {
      if (!spitfire::filesystem::MoveFile(sFilePathUFRawJPG, sFilePathJPG)) {
        LOG<<"cImageCacheManager::GetOrCreateThumbnailForDNGFile Failed to move the file from \""<<sFolderJPG<<"\" to \""<<sFilePathJPG<<"\", returning \"\""<<std::endl;
        return TEXT("");
      }
    } else {
      // Try to rename from file.jpg to abcdefghi_file.jpg
      const string_t sFileUFRawJPG = spitfire::filesystem::GetFileNoExtension(sDNGFilePath) + TEXT(".jpg");
      string_t sFilePathUFRawJPG = spitfire::filesystem::MakeFilePath(sFolderJPG, sFileUFRawJPG);
      LOG<<"cImageCacheManager::GetOrCreateThumbnailForDNGFile Looking for \""<<sFilePathUFRawJPG<<"\""<<std::endl;
      if (spitfire::filesystem::FileExists(sFilePathUFRawJPG) && !spitfire::filesystem::MoveFile(sFilePathUFRawJPG, sFilePathJPG)) {
        LOG<<"cImageCacheManager::GetOrCreateThumbnailForDNGFile Failed to move the file from \""<<sFolderJPG<<"\" to \""<<sFilePathJPG<<"\", returning \"\""<<std::endl;
        return TEXT("");
      }
    }

    if (!spitfire::filesystem::FileExists(sFilePathJPG)) {
      LOG<<"cImageCacheManager::GetOrCreateThumbnailForDNGFile Failed to create the thumbnail image \""<<sFilePathJPG<<"\", returning \"\""<<std::endl;
      return TEXT("");
    }

    return sFilePathJPG;
  }

  string_t cImageCacheManager::GetOrCreateThumbnailForImageFile(const string_t& sImageFilePath, IMAGE_SIZE imageSize)
  {
    LOG<<"cImageCacheManager::GetOrCreateThumbnailForImageFile \""<<sImageFilePath<<"\""<<std::endl;

    if (!IsConvertInstalled()) {
      LOG<<"cImageCacheManager::GetOrCreateThumbnailForImageFile convert is not installed, returning \"\""<<std::endl;
      return TEXT("");
    }

    spitfire::algorithm::cMD5 md5;
    md5.CalculateForFile(sImageFilePath);

    const string_t sCacheFolder = GetCacheFolderPath();

    string_t sFileJPG = TEXT("full.jpg");
    size_t width = 0;
    size_t height = 0;

    switch (imageSize) {
      case IMAGE_SIZE::THUMBNAIL: {
        sFileJPG = TEXT("thumbnail.jpg");
        width = 200;
        height = 200;
        break;
      }
    }

    const string_t sFilePathJPG = spitfire::filesystem::MakeFilePath(sCacheFolder, md5.GetResultFormatted() + TEXT("_") + sFileJPG);
    if (spitfire::filesystem::FileExists(sFilePathJPG)) return sFilePathJPG;

    const string_t sFolderJPG = spitfire::filesystem::GetFolder(sFilePathJPG);

    ostringstream_t o;
    o<<"convert \""<<sImageFilePath<<"\"";
    if ((width != 0) && (height != 0)) o<<" -resize "<<width<<"x"<<height;
    o<<" -auto-orient \""<<sFilePathJPG<<"\"";
    const string_t sCommandLine = o.str();
    const int iResult = system(sCommandLine.c_str());
    if (iResult != 0) {
      LOG<<"cImageCacheManager::GetOrCreateThumbnailForImageFile convert returned "<<iResult<<" for \""<<sCommandLine<<"\", returning \"\""<<std::endl;
      return TEXT("");
    }

    if (!spitfire::filesystem::FileExists(sFilePathJPG)) {
      LOG<<"cImageCacheManager::GetOrCreateThumbnailForImageFile Failed to create the thumbnail image \""<<sFilePathJPG<<"\", returning \"\""<<std::endl;
      return TEXT("");
    }

    return sFilePathJPG;
  }
}
