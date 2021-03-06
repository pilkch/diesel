// Standard headers
#include <string>

// libwin32mm headers
#include <libwin32mm/controls.h>
#include <libwin32mm/dialog.h>
#include <libwin32mm/filebrowse.h>

// Spitfire headers
#include <spitfire/storage/filesystem.h>
#include <spitfire/util/log.h>

// Diesel headers
#include "win32mmimportdialog.h"

namespace diesel
{
  class cImportDialog : public win32mm::cDialog
  {
  public:
    explicit cImportDialog(cSettings& settings);

    bool Run(win32mm::cWindow& parent);

  private:
    virtual void OnInit() override;
    virtual bool OnCommand(int iCommand) override;
    virtual bool OnOk() override;

    void MoveControlsInputAndButton(const win32mm::cInput& input, const win32mm::cButton& button, int x, int y, int width);

    void LayoutAndSetWindowSize();

    void UpdateExampleFilePath();

    void OnControlChanged();
    void OnFromOrToChanged();

    void OnBrowseFromFolder();
    void OnBrowseToFolder();

    cSettings& settings;

    // Controls
    // Import
    win32mm::cHorizontalLine lineImport;

    win32mm::cStatic importFromFolderText;
    win32mm::cInput importFromFolder;
    win32mm::cButton importBrowseFromFolder;

    win32mm::cStatic importToFolderText;
    win32mm::cInput importToFolder;
    win32mm::cButton importBrowseToFolder;

    win32mm::cStatic importExampleFilePathText;

    // After Import
    win32mm::cHorizontalLine lineAfterImport;
    win32mm::cCheckBox afterImportDeleteFromSourceFolderOnSuccessfulImport;

    win32mm::cHorizontalLine horizontalLine;
    win32mm::cButton buttonCancel;
    win32mm::cButton buttonOk;
  };

  cImportDialog::cImportDialog(cSettings& _settings) :
    settings(_settings)
  {
  }

  void cImportDialog::OnInit()
  {
    // Add controls
    lineImport.Create(*this, TEXT("Import"));

    importFromFolderText.Create(*this, TEXT("From:"));
    importFromFolder.Create(*this, 101);
    importBrowseFromFolder.Create(*this, 102, TEXT("Browse..."));

    importToFolderText.Create(*this, TEXT("To:"));
    importToFolder.Create(*this, 103);
    importBrowseToFolder.Create(*this, 104, TEXT("Browse..."));

    importExampleFilePathText.Create(*this, TEXT(""));

    // After Import
    lineAfterImport.Create(*this, TEXT("After Import"));
    afterImportDeleteFromSourceFolderOnSuccessfulImport.Create(*this, 105, TEXT("Delete from source folder on successful import"));

    // Add horizontal line
    horizontalLine.Create(*this);

    // Add standard buttons
    buttonOk.CreateOk(*this, TEXT("Ok"));
    buttonCancel.CreateCancel(*this);

    // Update the control values
    importFromFolder.SetValue(settings.GetLastImportFromFolder());
    importToFolder.SetValue(settings.GetLastImportToFolder());
    afterImportDeleteFromSourceFolderOnSuccessfulImport.SetChecked(settings.IsAfterImportDeleteFromSourceFolderOnSuccessfulImport());

    // Update our example text
    UpdateExampleFilePath();

    LayoutAndSetWindowSize();

    OnFromOrToChanged();
  }

  void cImportDialog::MoveControlsInputAndButton(const win32mm::cInput& input, const win32mm::cButton& button, int x, int y, int width)
  {
    const int widthButton = MeasureButtonWidth(button.GetHandle());
    const int widthInput = width - (widthButton + GetSpacerWidth());
    MoveControl(input.GetHandle(), x, y + ((GetButtonHeight() - GetInputHeight()) / 2), widthInput, GetInputHeight());
    x += widthInput + GetSpacerWidth();
    MoveControl(button.GetHandle(), x, y, widthButton, GetButtonHeight());
  }

  void cImportDialog::LayoutAndSetWindowSize()
  {
    int iX = GetMarginWidth();
    int iY = GetMarginHeight();

    // Find the widest static text
    const int widestStaticText = max(MeasureStaticTextWidth(importFromFolderText.GetHandle()), MeasureStaticTextWidth(importToFolderText.GetHandle()));

    // Find the widest row of controls on the right
    const int widestRowOnTheRight = max(DialogUnitsToPixelsX(180), MeasureCheckBoxWidth(afterImportDeleteFromSourceFolderOnSuccessfulImport.GetHandle()));
    const int widthControls = widestStaticText + GetSpacerWidth() + widestRowOnTheRight;
    const int widthImportText = MeasureStaticTextWidth(lineImport.GetStaticTextHandle());
    const int widthImportLine = widthControls - (widthImportText + GetSpacerWidth());
    const int heightImportExampleFilePathText = MeasureStaticTextHeight(importExampleFilePathText.GetHandle(), widthImportLine);
    const int staticTextHeight = MeasureStaticTextHeight(lineImport.GetStaticTextHandle(), widthImportLine);
    const int widthAfterImportText = MeasureStaticTextWidth(lineAfterImport.GetStaticTextHandle());
    const int lineHeightStaticInputButton = max(max(staticTextHeight, GetInputHeight()), GetButtonHeight());

    MoveControl(lineImport.GetStaticTextHandle(), iX, iY, widthImportText, staticTextHeight);
    MoveControl(lineImport.GetLineHandle(), iX + widthImportText + GetSpacerWidth(), iY + (staticTextHeight / 2), widthControls - (widthImportText + GetSpacerWidth()), 1);
    iY += staticTextHeight + GetSpacerHeight();

    MoveControlStaticNextToOtherControls(importFromFolderText.GetHandle(), iX, iY, widestStaticText);
    MoveControlsInputAndButton(importFromFolder, importBrowseFromFolder, iX + widestStaticText + GetSpacerWidth(), iY, widestRowOnTheRight);
    iY += lineHeightStaticInputButton + GetSpacerHeight();

    MoveControlStaticNextToOtherControls(importToFolderText.GetHandle(), iX, iY, widestStaticText);
    MoveControlsInputAndButton(importToFolder, importBrowseToFolder, iX + widestStaticText + GetSpacerWidth(), iY, widestRowOnTheRight);
    iY += lineHeightStaticInputButton + GetSpacerHeight();

    MoveControl(importExampleFilePathText.GetHandle(), iX, iY, widthImportLine, heightImportExampleFilePathText);
    iY += heightImportExampleFilePathText + GetSpacerHeight();
    iY += GetSpacerHeight();

    // After Import
    MoveControl(lineAfterImport.GetStaticTextHandle(), iX, iY, widthAfterImportText, staticTextHeight);
    MoveControl(lineAfterImport.GetLineHandle(), iX + widthAfterImportText + GetSpacerWidth(), iY + (staticTextHeight / 2), widthControls - (widthAfterImportText + GetSpacerWidth()), 1);
    iY += staticTextHeight + GetSpacerHeight();
    MoveControl(afterImportDeleteFromSourceFolderOnSuccessfulImport.GetHandle(), iX, iY, widthControls, GetCheckBoxHeight());
    iY += GetCheckBoxHeight() + GetSpacerHeight();

    // Horizontal line
    MoveControl(horizontalLine.GetLineHandle(), iX, iY, widthControls, 1);
    iY += 1 + GetSpacerHeight();

    const int iDialogWidth = GetMarginWidth() + widthControls + GetMarginWidth();
    const int iDialogHeight = iY + GetButtonHeight() + GetMarginHeight();

    MoveOkCancelHelp(iDialogWidth, iDialogHeight);

    // Set our window size now that we know how big it should be
    SetClientSize(iDialogWidth, iDialogHeight);
  }

  bool cImportDialog::OnCommand(int iCommand)
  {
    switch (iCommand) {
      // TODO: This doesn't work because it is too noisy, closing the a bubble tip triggers another bubble tip to be opened for example
      /*
      case 101:
      case 103: {
        OnFromOrToChanged();
        break;
      }*/
      case 102: {
        OnBrowseFromFolder();
        break;
      }
      case 104: {
        OnBrowseToFolder();
        break;
      }
    };

    return false;
  }

  bool cImportDialog::OnOk()
  {
    settings.SetLastImportFromFolder(importFromFolder.GetValue());
    settings.SetLastImportToFolder(importToFolder.GetValue());
    settings.SetAfterImportDeleteFromSourceFolderOnSuccessfulImport(afterImportDeleteFromSourceFolderOnSuccessfulImport.IsChecked());

    settings.Save();

    return true;
  }

  void cImportDialog::UpdateExampleFilePath()
  {
    ostringstream_t o;

    o<<TEXT("Example: ");

    const spitfire::util::cDateTime now;
    o<<now.GetYear()<<spitfire::filesystem::sFilePathSeparator;

    o<<now.GetDateYYYYMMDD()<<TEXT(" ");

    o<<TEXT("My Holiday Photos")<<spitfire::filesystem::sFilePathSeparator;

    importExampleFilePathText.SetText(o.str());
  }

  void cImportDialog::OnControlChanged()
  {
    UpdateExampleFilePath();
  }

  void cImportDialog::OnFromOrToChanged()
  {
    LOG<<"cImportDialog::OnFromOrToChanged"<<std::endl;

    if (!spitfire::filesystem::DirectoryExists(importFromFolder.GetValue())) {
      BubbleTipShow(importFromFolder.GetHandle(), TEXT("The from folder does not exist"));
      EnableControl(buttonOk.GetHandle(), false);
    } else if (!spitfire::filesystem::DirectoryExists(importToFolder.GetValue())) {
      BubbleTipShow(importToFolder.GetHandle(), TEXT("The to folder does not exist"));
      EnableControl(buttonOk.GetHandle(), false);
    } else {
      BubbleTipHide();
      EnableControl(buttonOk.GetHandle(), true);
    }
  }

  void cImportDialog::OnBrowseFromFolder()
  {
    // Browse for the folder
    win32mm::cFolderDialog dialog;
    dialog.SetType(win32mm::cFolderDialog::TYPE::SELECT);
    dialog.SetCaption(TEXT("Select from folder"));
    dialog.SetDefaultFolder(importFromFolder.GetValue());
    if (dialog.Run(*this)) {
      // Update the control
      importFromFolder.SetValue(dialog.GetSelectedFolder());

      // Enable or disable the Ok button
      OnFromOrToChanged();
    }
  }

  void cImportDialog::OnBrowseToFolder()
  {
    // Browse for the folder
    win32mm::cFolderDialog dialog;
    dialog.SetType(win32mm::cFolderDialog::TYPE::SELECT);
    dialog.SetCaption(TEXT("Select to folder"));
    dialog.SetDefaultFolder(importToFolder.GetValue());
    if (dialog.Run(*this)) {
      // Update the control
      importToFolder.SetValue(dialog.GetSelectedFolder());

      // Update the example file path
      UpdateExampleFilePath();

      // Enable or disable the Ok button
      OnFromOrToChanged();
    }
  }

  bool cImportDialog::Run(win32mm::cWindow& parent)
  {
    return RunNonResizable(parent, TEXT("Import"));
  }


  // ** OpenImportDialog

  bool OpenImportDialog(cSettings& settings, win32mm::cWindow& parent)
  {
    cImportDialog dialog(settings);
    return dialog.Run(parent);
  }
}
