#ifndef DIESEL_IMPORTTHREAD_H
#define DIESEL_IMPORTTHREAD_H

// Standard headers
#include <vector>

// Spitfire headers
#include <spitfire/util/process.h>
#include <spitfire/util/thread.h>

// Diesel headers
#include "diesel.h"

namespace diesel
{
  // Supported files
  //
  // Diesel imports raw, dng and image files

  class cImportProcess : public spitfire::util::cProcess
  {
  public:
    explicit cImportProcess(spitfire::util::cProcessInterface& interface);

    void SetFromFolder(const string_t& sFolder);
    void SetToFolder(const string_t& sFolder);
    void SetSeparateFolderForEachYear(bool bSeparateFolderForEachYear);
    void SetSeparateFolderForEachDate(bool bSeparateFolderForEachDate);
    void SetDescription(bool bDescription);
    void SetDescriptionText(const string_t& sDescription);
    void SetDeleteFromSourceFolderOnSuccessfulImport(bool bDelete);

    const std::vector<string_t>& GetImportedFolders() const;

  private:
    string_t GetFolderForFile(const string_t& sFromFilePath, const string_t& sToFolder) const;

    virtual spitfire::util::PROCESS_RESULT ProcessFunction() override;

    string_t sFromFolder;
    string_t sToFolder;
    bool bSeparateFolderForEachYear;
    bool bSeparateFolderForEachDate;
    bool bDescription;
    string_t sDescriptionText;
    bool bDeleteFromSourceFolderOnSuccessfulImport;

    std::vector<string_t> importedFolders;
  };
}

#endif // DIESEL_IMPORTTHREAD_H
