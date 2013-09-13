// Standard headers
#include <cassert>
#include <cmath>
#include <cstring>

#include <string>
#include <iostream>
#include <sstream>

#include <algorithm>
#include <map>
#include <vector>
#include <list>

// Spitfire headers
#include <spitfire/math/math.h>
#include <spitfire/storage/filesystem.h>

// Diesel headers
#include "settings.h"

namespace diesel
{
  void cSettings::Load()
  {
    document.Load();
  }

  void cSettings::Save()
  {
    document.Save();
  }

  void cSettings::GetMainWindowSize(size_t& width, size_t& height) const
  {
    width = document.GetValue<size_t>(TEXT("settings"), TEXT("mainWindow"), TEXT("width"), 1000);
    width = spitfire::math::clamp<size_t>(width, 400, 2000);
    height = document.GetValue<size_t>(TEXT("settings"), TEXT("mainWindow"), TEXT("height"), 1000);
    height = spitfire::math::clamp<size_t>(height, 400, 2000);
  }

  void cSettings::SetMainWindowSize(size_t width, size_t height)
  {
    document.SetValue<size_t>(TEXT("settings"), TEXT("mainWindow"), TEXT("width"), width);
    document.SetValue<size_t>(TEXT("settings"), TEXT("mainWindow"), TEXT("height"), height);
  }

  bool cSettings::IsMainWindowMaximised() const
  {
    return document.GetValue<bool>(TEXT("settings"), TEXT("mainWindow"), TEXT("maximised"), false);
  }

  void cSettings::SetMainWindowMaximised(bool bMaximised)
  {
    document.SetValue(TEXT("settings"), TEXT("mainWindow"), TEXT("maximised"), bMaximised);
  }

  string_t cSettings::GetIgnoreUpdateVersion() const
  {
    return document.GetValue<string_t>(TEXT("settings"), TEXT("update"), TEXT("ignoredVersion"), "");
  }

  void cSettings::SetIgnoreUpdateVersion(const string_t& sVersion)
  {
    document.SetValue(TEXT("settings"), TEXT("update"), TEXT("ignoredVersion"), sVersion);
  }

  size_t cSettings::GetMaximumCacheSizeGB() const
  {
    return document.GetValue<size_t>(TEXT("settings"), TEXT("cache"), TEXT("maximumSizeGB"), 2);
  }

  void cSettings::SetMaximumCacheSizeGB(size_t nSizeGB)
  {
    document.SetValue(TEXT("settings"), TEXT("cache"), TEXT("maximumSizeGB"), nSizeGB);
  }

  void cSettings::GetPreviousPhotoBrowserFolders(std::list<string_t>& folders) const
  {
    std::vector<string_t> vFolders;
    document.GetListOfValues(TEXT("settings"), TEXT("path"), TEXT("recentPhotoBrowserFolder"), vFolders);

    const size_t n = vFolders.size();
    for (size_t i = 0; i < n; i++) folders.push_back(vFolders[i]);
  }

  void cSettings::SetPreviousPhotoBrowserFolders(const std::list<string_t>& folders)
  {
    std::vector<string_t> vFolders;
    std::list<string_t>::const_iterator iter(folders.begin());
    const std::list<string_t>::const_iterator iterEnd(folders.end());
    while (iter != iterEnd) {
      vFolders.push_back(*iter);

      iter++;
    }

    document.SetListOfValues(TEXT("settings"), TEXT("path"), TEXT("recentPhotoBrowserFolder"), vFolders);
  }

  string_t cSettings::GetLastPhotoBrowserFolder() const
  {
    return document.GetValue<string_t>(TEXT("settings"), TEXT("path"), TEXT("lastPhotoBrowserFolder"), spitfire::filesystem::GetHomePicturesDirectory());
  }

  void cSettings::SetLastPhotoBrowserFolder(const string_t& sLastPhotoBrowserFolder)
  {
    document.SetValue(TEXT("settings"), TEXT("path"), TEXT("lastPhotoBrowserFolder"), sLastPhotoBrowserFolder);
  }

  string_t cSettings::GetLastAddLocation() const
  {
    return document.GetValue<string_t>(TEXT("settings"), TEXT("path"), TEXT("lastAddLocation"), spitfire::filesystem::GetHomeMusicDirectory());
  }

  void cSettings::SetLastAddLocation(const string_t& sLastAddLocation)
  {
    document.SetValue(TEXT("settings"), TEXT("path"), TEXT("lastAddLocation"), sLastAddLocation);
  }


  string_t cSettings::GetLastImportFromFolder() const
  {
    return document.GetValue<string_t>(TEXT("settings"), TEXT("import"), TEXT("lastImportFromFolder"), TEXT("/media/"));
  }

  void cSettings::SetLastImportFromFolder(const string_t& sFolder)
  {
    document.SetValue(TEXT("settings"), TEXT("import"), TEXT("lastImportFromFolder"), sFolder);
  }

  string_t cSettings::GetLastImportToFolder() const
  {
    return document.GetValue<string_t>(TEXT("settings"), TEXT("import"), TEXT("lastImportToFolder"), spitfire::filesystem::GetHomePicturesDirectory());
  }

  void cSettings::SetLastImportToFolder(const string_t& sFolder)
  {
    document.SetValue(TEXT("settings"), TEXT("import"), TEXT("lastImportToFolder"), sFolder);
  }

  bool cSettings::IsImportSeparateFolderForEachYear() const
  {
    return document.GetValue<bool>(TEXT("settings"), TEXT("import"), TEXT("isSeparateFolderForEachYear"), true);
  }

  void cSettings::SetImportSeparateFolderForEachYear(bool bSeparateFolderForEachYear)
  {
    document.SetValue(TEXT("settings"), TEXT("import"), TEXT("isSeparateFolderForEachYear"), bSeparateFolderForEachYear);
  }

  bool cSettings::IsImportSeparateFolderForEachDate() const
  {
    return document.GetValue<bool>(TEXT("settings"), TEXT("import"), TEXT("isSeparateFolderForEachDate"), true);
  }

  void cSettings::SetImportSeparateFolderForEachDate(bool bSeparateFolderForEachDate)
  {
    document.SetValue(TEXT("settings"), TEXT("import"), TEXT("isSeparateFolderForEachDate"), bSeparateFolderForEachDate);
  }

  bool cSettings::IsImportDescription() const
  {
    return document.GetValue<bool>(TEXT("settings"), TEXT("import"), TEXT("isImportDescription"), false);
  }

  void cSettings::SetImportDescription(bool bDescription)
  {
    document.SetValue(TEXT("settings"), TEXT("import"), TEXT("isImportDescription"), bDescription);
  }

  string_t cSettings::GetImportDescriptionText() const
  {
    return document.GetValue<string_t>(TEXT("settings"), TEXT("import"), TEXT("descriptionText"), TEXT(""));
  }

  void cSettings::SetImportDescriptionText(const string_t& sDescription)
  {
    document.SetValue(TEXT("settings"), TEXT("import"), TEXT("descriptionText"), sDescription);
  }

  bool cSettings::IsAfterImportDeleteFromSourceFolderOnSuccessfulImport() const
  {
    return document.GetValue<bool>(TEXT("settings"), TEXT("import"), TEXT("isDeleteFromSourceFolderOnSuccessfulImport"), false);
  }

  void cSettings::SetAfterImportDeleteFromSourceFolderOnSuccessfulImport(bool bDelete)
  {
    document.SetValue(TEXT("settings"), TEXT("import"), TEXT("isDeleteFromSourceFolderOnSuccessfulImport"), bDelete);
  }
}
