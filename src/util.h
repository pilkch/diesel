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

    bool GenerateThumbnail(voodoo::cImage& thumbnail, const voodoo::cImage& image, size_t width, size_t height);


    // Inlines

    inline bool IsFileTypeRaw(const string_t& sExtensionLower)
    {
      return (
        (sExtensionLower == ".cr2") || (sExtensionLower == ".crw") ||
        (sExtensionLower == ".nef") || (sExtensionLower == ".nrw") ||
        (sExtensionLower == ".pef") || (sExtensionLower == ".ptx") ||
        (sExtensionLower == ".raw")
      );
    }

    inline bool IsFileTypeImage(const string_t& sExtensionLower)
    {
      return (
        (sExtensionLower == ".bmp") ||
        (sExtensionLower == ".jpeg") ||
        (sExtensionLower == ".jpg") ||
        (sExtensionLower == ".png")
      );
    }

    inline bool IsFileTypeSupported(const string_t& sExtensionLower)
    {
      return IsFileTypeRaw(sExtensionLower) || (sExtensionLower == ".dng") || IsFileTypeImage(sExtensionLower);
    }



    template <size_t N>
    inline const string_t FindFileExtension(const string_t& sFolderPath, const string_t& sFileNameNoExtension, const string_t(& extensions)[N])
    {
      for (size_t i = 0; i < N; i++) {
        if (spitfire::filesystem::FileExists(spitfire::filesystem::MakeFilePath(sFolderPath, sFileNameNoExtension + extensions[i]))) return extensions[i];
      }

      return "";
    }

    inline string_t FindFileExtensionForRawFile(const string_t& sFolderPath, const string_t& sFileNameNoExtension)
    {
      const string_t extensions[] = {
        ".cr2",
        ".crw",
        ".nef",
        ".nrw",
        ".pef",
        ".ptx",
        ".raw",
      };
      return FindFileExtension(sFolderPath, sFileNameNoExtension, extensions);
    }

    inline string_t FindFileExtensionForImageFile(const string_t& sFolderPath, const string_t& sFileNameNoExtension)
    {
      // NOTE: These are in order of what we prefer to work with in case there are multiple image files with this name but a different extension
      const string_t extensions[] = {
        ".bmp",
        ".png",
        ".jpeg",
        ".jpg",
      };
      return FindFileExtension(sFolderPath, sFileNameNoExtension, extensions);
    }
  }
}

#endif // DIESEL_UTIL_H
