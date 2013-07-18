#ifndef DIESEL_SETTINGS_H
#define DIESEL_SETTINGS_H

// Standard headers
#include <list>

// Spitfire headers
#include <spitfire/storage/settings.h>

// Diesel headers
#include "diesel.h"

namespace diesel
{
  // ** cSettings

  class cSettings
  {
  public:
    void Load();
    void Save();

    string_t GetIgnoreUpdateVersion() const;
    void SetIgnoreUpdateVersion(const string_t& sVersion);

    void GetPreviousPhotoBrowserFolders(std::list<string_t>& folders) const;
    void SetPreviousPhotoBrowserFolders(const std::list<string_t>& folders);

    string_t GetLastPhotoBrowserFolder() const;
    void SetLastPhotoBrowserFolder(const string_t& sLastPhotoBrowserFolder);

    string_t GetLastAddLocation() const;
    void SetLastAddLocation(const string_t& sLastAddLocation);

  private:
    spitfire::storage::cSettingsDocument document;
  };
}

#endif // DIESEL_SETTINGS_H
