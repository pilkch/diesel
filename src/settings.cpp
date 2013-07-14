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

  string_t cSettings::GetIgnoreUpdateVersion() const
  {
    return document.GetValue<string_t>(TEXT("settings"), TEXT("update"), TEXT("ignoredVersion"), "");
  }

  void cSettings::SetIgnoreUpdateVersion(const string_t& sVersion)
  {
    document.SetValue(TEXT("settings"), TEXT("update"), TEXT("ignoredVersion"), sVersion);
  }

  void cSettings::GetPreviousPhotoBrowserFolders(std::list<string_t>& folders)
  {
    folders.clear();

    // Get the count
    const size_t n = document.GetValue(TEXT("settings"), TEXT("path"), TEXT("recentPhotoBrowserFolderCount"), 0);

    // Get each path
    string_t sValue;
    for (size_t i = 0; i < n; i++) {
      sValue = document.GetValue<string_t>(TEXT("settings"), TEXT("path"), TEXT("recentPhotoBrowserFolder") + spitfire::string::ToString(i), TEXT(""));
      if (!sValue.empty()) folders.push_back(sValue);
    }
  }

  void cSettings::SetPreviousPhotoBrowserFolders(const std::list<string_t>& folders)
  {
    // Set the count
    const size_t n = folders.size();
    document.SetValue(TEXT("settings"), TEXT("path"), TEXT("recentPhotoBrowserFolderCount"), n);

    // Add each path
    size_t i = 0;
    std::list<string_t>::const_iterator iter(folders.begin());
    const std::list<string_t>::const_iterator iterEnd(folders.end());
    while (iter != iterEnd) {
      document.SetValue(TEXT("settings"), TEXT("path"), TEXT("recentPhotoBrowserFolder") + spitfire::string::ToString(i), *iter);

      iter++;
    }
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
}
