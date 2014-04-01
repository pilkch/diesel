// Spitfire headers
#include <spitfire/algorithm/md5.h>
#include <spitfire/storage/filesystem.h>
#include <spitfire/util/log.h>

// Diesel headers
#include "importthread.h"
#include "util.h"

namespace diesel
{
  cImportProcess::cImportProcess(spitfire::util::cProcessInterface& interface) :
    spitfire::util::cProcess(interface),
    bSeparateFolderForEachYear(false),
    bSeparateFolderForEachDate(false),
    bDescription(false),
    bDeleteFromSourceFolderOnSuccessfulImport(false)
  {
  }

  void cImportProcess::SetFromFolder(const string_t& _sFromFolder)
  {
    sFromFolder = _sFromFolder;
  }

  void cImportProcess::SetToFolder(const string_t& _sToFolder)
  {
    sToFolder = _sToFolder;
  }

  void cImportProcess::SetSeparateFolderForEachYear(bool _bSeparateFolderForEachYear)
  {
    bSeparateFolderForEachYear = _bSeparateFolderForEachYear;
  }

  void cImportProcess::SetSeparateFolderForEachDate(bool _bSeparateFolderForEachDate)
  {
    bSeparateFolderForEachDate = _bSeparateFolderForEachDate;
  }

  void cImportProcess::SetDescription(bool _bDescription)
  {
    bDescription = _bDescription;
  }

  void cImportProcess::SetDescriptionText(const string_t& _sDescriptionText)
  {
    sDescriptionText = _sDescriptionText;
  }

  void cImportProcess::SetDeleteFromSourceFolderOnSuccessfulImport(bool _bDelete)
  {
    bDeleteFromSourceFolderOnSuccessfulImport = _bDelete;
  }

  string_t cImportProcess::GetFolderForFile(const string_t& sFromFilePath, const string_t& sToFolder) const
  {
    ostringstream_t o;

    o<<sToFolder;
    if (sToFolder.back() != spitfire::filesystem::cFilePathSeparator) o<<spitfire::filesystem::sFilePathSeparator;
    const spitfire::util::cDateTime dateTime = spitfire::filesystem::GetLastModifiedDate(sFromFilePath);
    if (bSeparateFolderForEachYear) o<<dateTime.GetYear()<<spitfire::filesystem::sFilePathSeparator;

    if (bDescription && !sDescriptionText.empty()) {
      if (bSeparateFolderForEachDate) o<<dateTime.GetDateYYYYMMDD()<<" ";

      o<<sDescriptionText<<spitfire::filesystem::sFilePathSeparator;
    } else if (bSeparateFolderForEachDate) o<<dateTime.GetDateYYYYMMDD()<<spitfire::filesystem::sFilePathSeparator;

    return o.str();
  }

  spitfire::util::PROCESS_RESULT cImportProcess::ProcessFunction()
  {
    LOG<<"cImportProcess::ProcessFunction \""<<sFromFolder<<"\""<<std::endl;

    interface.SetCancellable(true);
    interface.SetTextPrimary(TEXT("Importing..."));

    interface.SetPercentageCompletePrimary0To100(0.0f);
    interface.SetPercentageCompleteSecondary0To100(0.0f);

    ASSERT(spitfire::filesystem::DirectoryExists(sFromFolder));
    ASSERT(spitfire::filesystem::DirectoryExists(sToFolder));

    // Import the files
    {
      interface.SetTextSecondary(TEXT("Copying files..."));

      spitfire::filesystem::cFolderIterator iter(sFromFolder);
      size_t i = 0;
      const size_t n = iter.GetFileAndFolderCount();
      while (iter.IsValid() && !interface.IsToStop()) {
        interface.SetPercentageCompleteSecondary0To100((float(i) / float(n)) * 100.0f);

        const string_t sFromFilePath = iter.GetFullPath();
        LOG<<"cImportProcess::ProcessFunction From \""<<sFromFilePath<<"\""<<std::endl;
        if (spitfire::filesystem::IsFile(sFromFilePath) && util::IsFileTypeSupported(spitfire::string::ToLower(spitfire::filesystem::GetExtension(sFromFilePath)))) {
          const string_t sToFullFolder = GetFolderForFile(sFromFilePath, sToFolder);
          if (!spitfire::filesystem::DirectoryExists(sToFullFolder)) spitfire::filesystem::CreateDirectory(sToFullFolder);
          const string_t sToFilePath = spitfire::filesystem::MakeFilePath(sToFullFolder, spitfire::filesystem::GetFile(sFromFilePath));
          LOG<<"cImportProcess::ProcessFunction Copying from \""<<sFromFilePath<<"\" to \""<<sToFilePath<<"\""<<std::endl;
          spitfire::filesystem::CopyFile(sFromFilePath, sToFilePath);
        }

        iter.Next();
        i++;
      }

      if (interface.IsToStop()) return spitfire::util::PROCESS_RESULT::STOPPED_BY_INTERFACE;
    }

    interface.SetPercentageCompletePrimary0To100(50.0f);
    interface.SetPercentageCompleteSecondary0To100(0.0f);

    // Check if the MD5 hashes match and we can delete the original files
    if (bDeleteFromSourceFolderOnSuccessfulImport) {
      interface.SetTextSecondary(TEXT("Deleting source files..."));

      spitfire::algorithm::cMD5 md5;

      spitfire::filesystem::cFolderIterator iter(sFromFolder);
      size_t i = 0;
      const size_t n = iter.GetFileAndFolderCount();
      while (iter.IsValid() && !interface.IsToStop()) {
        interface.SetPercentageCompleteSecondary0To100((float(i) / float(n)) * 100.0f);

        const string_t sFromFilePath = iter.GetFullPath();
        if (spitfire::filesystem::IsFile(sFromFilePath) && util::IsFileTypeSupported(spitfire::string::ToLower(spitfire::filesystem::GetExtension(sFromFilePath)))) {
          const string_t sToFullFolder = GetFolderForFile(sFromFilePath, sToFolder);
          if (!spitfire::filesystem::DirectoryExists(sToFullFolder)) spitfire::filesystem::CreateDirectory(sToFullFolder);
          const string_t sToFilePath = spitfire::filesystem::MakeFilePath(sToFullFolder, spitfire::filesystem::GetFile(sFromFilePath));

          // Check the MD5 hashes
          string_t sFromMD5;
          if (md5.CalculateForFile(sFromFilePath)) sFromMD5 = md5.GetResultFormatted();
          string_t sToMD5;
          if (md5.CalculateForFile(sToFilePath)) sToMD5 = md5.GetResultFormatted();

          if (!sFromMD5.empty() && !sToMD5.empty() && (sToMD5 == sFromMD5)) {
            // Delete the source file
            spitfire::filesystem::DeleteFile(sFromFilePath);
          }
        }

        iter.Next();
        i++;
      }

      if (interface.IsToStop()) return spitfire::util::PROCESS_RESULT::STOPPED_BY_INTERFACE;
    }

    interface.SetPercentageCompletePrimary0To100(100.0f);
    interface.SetPercentageCompleteSecondary0To100(100.0f);

    LOG<<"cImportProcess::ProcessFunction returning PROCESS_RESULT::COMPLETE"<<std::endl;
    return spitfire::util::PROCESS_RESULT::COMPLETE;
  }
}
