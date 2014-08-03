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

namespace diesel
{
  #ifdef __WIN__

  // ** cPipe
  //
  // http://msdn.microsoft.com/en-us/library/ms682499%28VS.85%29.aspx
  //

  #define BUFSIZE 4096

  class cPipe
  {
  public:
    cPipe();

    void RunCommandLine(const spitfire::string_t& sCommandLine);

  private:
    void CreateChildProcess(const spitfire::string_t& sCommandLine);
    void ReadFromPipe();

    HANDLE hChildStd_IN_Rd;
    HANDLE hChildStd_IN_Wr;
    HANDLE hChildStd_OUT_Rd;
    HANDLE hChildStd_OUT_Wr;

    HANDLE hChildProcess;
  };

  cPipe::cPipe() :
    hChildStd_IN_Rd(NULL),
    hChildStd_IN_Wr(NULL),
    hChildStd_OUT_Rd(NULL),
    hChildStd_OUT_Wr(NULL),
    hChildProcess(NULL)
  {
  }

  // Create a child process that uses the previously created pipes for STDIN and STDOUT.
  void cPipe::CreateChildProcess(const spitfire::string_t& sCommandLine)
  {
    // Set up members of the PROCESS_INFORMATION structure
    PROCESS_INFORMATION piProcInfo;
    ZeroMemory(&piProcInfo, sizeof(PROCESS_INFORMATION));

    // Set up members of the STARTUPINFO structure
    // This structure specifies the STDIN and STDOUT handles for redirection.
    STARTUPINFO siStartInfo;
    ZeroMemory(&siStartInfo, sizeof(STARTUPINFO));
    siStartInfo.cb = sizeof(STARTUPINFO);
    siStartInfo.hStdError = hChildStd_OUT_Wr;
    siStartInfo.hStdOutput = hChildStd_OUT_Wr;
    siStartInfo.hStdInput = hChildStd_IN_Rd;
    siStartInfo.dwFlags |= STARTF_USESTDHANDLES;

    // Create the child process
    spitfire::string_t sCommandLineMutable = sCommandLine;
    BOOL bSuccess = ::CreateProcess(NULL,
      (spitfire::char_t*)sCommandLineMutable.c_str(),     // command line
      NULL,          // process security attributes
      NULL,          // primary thread security attributes
      TRUE,          // handles are inherited
      CREATE_NO_WINDOW, // Creation flags, hide the console window
      NULL,          // use parent's environment
      NULL,          // use parent's current directory
      &siStartInfo,  // STARTUPINFO pointer
      &piProcInfo    // receives PROCESS_INFORMATION
    );

    // If an error occurs, exit the application
    if (!bSuccess) {
      LOGERROR<<"CreateChildProcess CreateProcess FAILED, returning"<<std::endl;
      return;
    }

    // Close handles to the child's primary thread
    ::CloseHandle(piProcInfo.hThread);

    // Remember the child process so that we can wait for it to finish
    hChildProcess = piProcInfo.hProcess;
  }

  // Read output from the child process's pipe for STDOUT
  // and write to the parent process's pipe for STDOUT.
  // Stop when there is no more data.
  void cPipe::ReadFromPipe()
  {
    DWORD dwRead = 0;
    DWORD dwWritten = 0;
    CHAR chBuf[BUFSIZE];
    BOOL bSuccess = FALSE;
    HANDLE hParentStdOut = ::GetStdHandle(STD_OUTPUT_HANDLE);

    while (true) {
      bSuccess = ::ReadFile(hChildStd_OUT_Rd, chBuf, BUFSIZE, &dwRead, NULL);
      if (!bSuccess || (dwRead == 0)) break;

      bSuccess = ::WriteFile(hParentStdOut, chBuf, dwRead, &dwWritten, NULL);
      if (!bSuccess) break;
    }
  }

  void cPipe::RunCommandLine(const diesel::string_t& sCommandLine)
  {
    CreateChildProcess(sCommandLine);

    ::CloseHandle(hChildStd_OUT_Wr);

    ReadFromPipe();

    ::CloseHandle(hChildStd_OUT_Rd);

    // Wait for the child to finish
    ::WaitForSingleObject(hChildProcess, INFINITE);

    hChildProcess = NULL;
  }

  void RunCommandLine(const diesel::string_t& sCommandLine)
  {
    cPipe pipe;
    pipe.RunCommandLine(sCommandLine);
  }
  #endif

  #ifdef __WIN__
  const string_t sFolderSeparator = TEXT("\\");
  #else
  const string_t sFolderSeparator = TEXT("/");
  #endif

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

  #ifdef __WIN__
  string_t cImageCacheManager::GetUFRawBatchPath()
  {
    const string_t sProgramFiles = spitfire::filesystem::GetProgramFilesDirectory();
    const string_t sUFRawBin = spitfire::filesystem::MakeFilePath(sProgramFiles, TEXT("UFRaw"), TEXT("bin"));
    const string_t sExecutablePath = spitfire::filesystem::MakeFilePath(sUFRawBin, TEXT("ufraw-batch.exe"));
    return sExecutablePath;
  }

  string_t cImageCacheManager::GetConvertPath()
  {
    // For some reason the 64 bit version installs into "Program Files" too
    const string_t sProgramFiles = TEXT("C:\\Program Files\\");
    for (spitfire::filesystem::cFolderIterator iter(sProgramFiles); iter.IsValid(); iter.Next()) {
      if (spitfire::string::StartsWith(iter.GetFileOrFolder(), TEXT("ImageMagick"))) {
        const string_t sImageMagickPath = iter.GetFullPath();
        return spitfire::filesystem::MakeFilePath(sImageMagickPath, TEXT("convert.exe"));
      }
    }

    return TEXT("");
  }
  #endif

  string_t cImageCacheManager::GetAdobeDNGConverterPath()
  {
    #ifdef __WIN__
    const string_t sProgramFiles = spitfire::filesystem::GetProgramFilesDirectory();
    const string_t sExecutablePath = spitfire::filesystem::MakeFilePath(sProgramFiles, TEXT("Adobe"), TEXT("Adobe DNG Converter.exe"));
    return sExecutablePath;
    #else
    return TEXT("C:\\Program Files (x86)\\Adobe\\Adobe DNG Converter.exe");
    #endif
  }

  #ifndef __WIN__
  bool cImageCacheManager::IsWineInstalled()
  {
    const int iResult = system("which wine");
    LOG<<"cImageCacheManager::IsWineInstalled returned "<<iResult<<std::endl;
    return (iResult == 0);
  }
  #endif

  #ifdef __WIN__
  bool cImageCacheManager::IsAdobeDNGConverterInstalled()
  {
    const string_t sAdobeDNGConverterPath = GetAdobeDNGConverterPath();
    const bool bExists = spitfire::filesystem::FileExists(sAdobeDNGConverterPath);
    LOG<<"cImageCacheManager::IsAdobeDNGConverterInstalled returning "<<spitfire::string::ToString(bExists)<<std::endl;
    return bExists;
  }

  bool cImageCacheManager::IsUFRawBatchInstalled()
  {
    const string_t sUFRawBatchPath = GetUFRawBatchPath();
    const bool bExists = spitfire::filesystem::FileExists(sUFRawBatchPath);
    LOG<<"cImageCacheManager::IsUFRawBatchInstalled returning "<<spitfire::string::ToString(bExists)<<std::endl;
    return bExists;
  }

  bool cImageCacheManager::IsConvertInstalled()
  {
    const string_t sConvertPath = GetConvertPath();
    const bool bExists = spitfire::filesystem::FileExists(sConvertPath);
    LOG<<"cImageCacheManager::IsConvertInstalled returning "<<spitfire::string::ToString(bExists)<<std::endl;
    return bExists;
  }
  #else
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
  #endif

  string_t cImageCacheManager::GetOrCreateDNGForRawFile(const string_t& sRawFilePath)
  {
    LOG<<"cImageCacheManager::GetOrCreateDNGForRawFile \""<<sRawFilePath<<"\""<<std::endl;

    ASSERT(spitfire::filesystem::FileExists(sRawFilePath));

    #ifndef __WIN__
    if (!IsWineInstalled()) {
      LOG<<"cImageCacheManager::GetOrCreateDNGForRawFile wine is not installed, returning \"\""<<std::endl;
      return TEXT("");
    }
    #endif

    #ifdef __WIN__
    // TODO: Use the actual dng sdk instead?
    #else
    // TODO: Use the actual dng sdk like this instead?
    // https://projects.kde.org/projects/extragear/graphics/kipi-plugins/repository/revisions/master/show/dngconverter
    #endif

    const string_t sFolder = spitfire::filesystem::GetFolder(sRawFilePath);
    const string_t sFile = spitfire::filesystem::GetFileNoExtension(sRawFilePath);
    const string_t sDNGFilePath = spitfire::filesystem::MakeFilePath(sFolder, sFile + TEXT(".dng"));

    ostringstream_t o;
    #ifndef __WIN__
    o<<"wine ";
    #endif
    const string_t sExecutablePath = GetAdobeDNGConverterPath();
    o<<"\""<<sExecutablePath<<"\" -c \""<<sRawFilePath<<"\"";
    #ifndef BUILD_DEBUG
    o<<" &> /dev/null";
    #endif
    const string_t sCommandLine = o.str();
    #ifdef __WIN__
    RunCommandLine(sCommandLine);
    if (!spitfire::filesystem::FileExists(sDNGFilePath)) {
      LOG<<"cImageCacheManager::GetOrCreateDNGForRawFile \""<<sCommandLine<<"\" FAILED, returning \"\""<<std::endl;
      return TEXT("");
    }
    #else
    const int iResult = system(sCommandLine.c_str());
    if (iResult != 0) {
      LOG<<"cImageCacheManager::GetOrCreateDNGForRawFile \""<<sCommandLine<<"\" returned "<<iResult<<", returning \"\""<<std::endl;
      return TEXT("");
    }
    #endif

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
    #ifdef __WIN__
    o<<"\""<<GetUFRawBatchPath()<<"\"";
    #else
    o<<"ufraw-batch";
    #endif
    o<<" --out-type=jpg";
    if (size != 0) o<<" --embedded-image --size="<<size;
    o<<" \""<<sDNGFilePath<<"\" --overwrite --out-path=\""<<spitfire::string::StripTrailing(sFolderJPG, sFolderSeparator)<<"\"";
    #ifndef BUILD_DEBUG
    o<<" --silent";
    #endif
    const string_t sCommandLine = o.str();
    LOG<<"cImageCacheManager::GetOrCreateThumbnailForDNGFile Running command line \""<<sCommandLine<<"\""<<std::endl;
    #ifdef __WIN__
    RunCommandLine(sCommandLine);
    if (!spitfire::filesystem::FileExists(sFilePathUFRawJPG)) {
      LOG<<"cImageCacheManager::GetOrCreateThumbnailForDNGFile ufraw-batch FAILED for \""<<sCommandLine<<"\", returning \"\""<<std::endl;
      return TEXT("");
    }
    #else
    const int iResult = system(sCommandLine.c_str());
    if (iResult != 0) {
      LOG<<"cImageCacheManager::GetOrCreateThumbnailForDNGFile ufraw-batch returned "<<iResult<<" for \""<<sCommandLine<<"\", returning \"\""<<std::endl;
      return TEXT("");
    }
    #endif

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
    o<<"\""<<GetConvertPath()<<"\" \""<<sImageFilePath<<"\"";
    if ((width != 0) && (height != 0)) o<<" -resize "<<width<<"x"<<height;
    o<<" -auto-orient \""<<sFilePathJPG<<"\"";
    const string_t sCommandLine = o.str();
    #ifdef __WIN__
    RunCommandLine(sCommandLine);
    #else
    const int iResult = system(sCommandLine.c_str());
    if (iResult != 0) {
      LOG<<"cImageCacheManager::GetOrCreateThumbnailForImageFile convert returned "<<iResult<<" for \""<<sCommandLine<<"\", returning \"\""<<std::endl;
      return TEXT("");
    }
    #endif

    if (!spitfire::filesystem::FileExists(sFilePathJPG)) {
      LOG<<"cImageCacheManager::GetOrCreateThumbnailForImageFile Failed to create the thumbnail image \""<<sFilePathJPG<<"\", returning \"\""<<std::endl;
      return TEXT("");
    }

    return sFilePathJPG;
  }
}
