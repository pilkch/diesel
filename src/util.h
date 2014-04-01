#ifndef DIESEL_UTIL_H
#define DIESEL_UTIL_H

// libvoodoomm headers
#include <libvoodoomm/cImage.h>

// Spitfire headers
#include <spitfire/storage/filesystem.h>

// Diesel headers
#include "diesel.h"

namespace diesel
{
  namespace util
  {
    bool IsFileTypeRaw(const string_t& sExtensionLower); // Nef, crw, etc.
    bool IsFileTypeImage(const string_t& sExtensionLower); // Jpg, png, etc.
    bool IsFileTypeSupported(const string_t& sExtensionLower); // Raw, dng or image

    string_t FindFileExtensionForRawFile(const string_t& sFolderPath, const string_t& sFileNameNoExtension);
    string_t FindFileExtensionForImageFile(const string_t& sFolderPath, const string_t& sFileNameNoExtension);


    // Inlines

    inline bool IsFileTypeRaw(const string_t& sExtensionLower)
    {
      return (
        (sExtensionLower == TEXT(".cr2")) || (sExtensionLower == TEXT(".crw")) ||
        (sExtensionLower == TEXT(".nef")) || (sExtensionLower == TEXT(".nrw")) ||
        (sExtensionLower == TEXT(".pef")) || (sExtensionLower == TEXT(".ptx")) ||
        (sExtensionLower == TEXT(".raw"))
      );
    }

    inline bool IsFileTypeImage(const string_t& sExtensionLower)
    {
      return (
        (sExtensionLower == TEXT(".bmp")) ||
        (sExtensionLower == TEXT(".jpeg")) ||
        (sExtensionLower == TEXT(".jpg")) ||
        (sExtensionLower == TEXT(".png"))
      );
    }

    inline bool IsFileTypeSupported(const string_t& sExtensionLower)
    {
      return IsFileTypeRaw(sExtensionLower) || (sExtensionLower == TEXT(".dng")) || IsFileTypeImage(sExtensionLower);
    }



    template <size_t N>
    inline const string_t FindFileExtension(const string_t& sFolderPath, const string_t& sFileNameNoExtension, const string_t(& extensions)[N])
    {
      for (size_t i = 0; i < N; i++) {
        if (spitfire::filesystem::FileExists(spitfire::filesystem::MakeFilePath(sFolderPath, sFileNameNoExtension + extensions[i]))) return extensions[i];
      }

      return TEXT("");
    }

    inline string_t FindFileExtensionForRawFile(const string_t& sFolderPath, const string_t& sFileNameNoExtension)
    {
      const string_t extensions[] = {
        TEXT(".cr2"),
        TEXT(".crw"),
        TEXT(".nef"),
        TEXT(".nrw"),
        TEXT(".pef"),
        TEXT(".ptx"),
        TEXT(".raw"),
      };
      return FindFileExtension(sFolderPath, sFileNameNoExtension, extensions);
    }

    inline string_t FindFileExtensionForImageFile(const string_t& sFolderPath, const string_t& sFileNameNoExtension)
    {
      // NOTE: These are in order of what we prefer to work with in case there are multiple image files with this name but a different extension
      const string_t extensions[] = {
        TEXT(".bmp"),
        TEXT(".png"),
        TEXT(".jpeg"),
        TEXT(".jpg"),
      };
      return FindFileExtension(sFolderPath, sFileNameNoExtension, extensions);
    }
  }
}

#endif // DIESEL_UTIL_H
