#ifndef DIESEL_SETTINGS_H
#define DIESEL_SETTINGS_H

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

    string_t GetLastAddLocation() const;
    void SetLastAddLocation(const string_t& sLastAddLocation);

  private:
    spitfire::storage::cSettingsDocument document;
  };
}

#endif // DIESEL_SETTINGS_H
