// Standard headers
#include <string>

// libwin32mm headers
#include <libwin32mm/controls.h>
#include <libwin32mm/dialog.h>

// Spitfire headers
#include <spitfire/util/log.h>

// Diesel headers
#include "win32mmsettingsdialog.h"

namespace diesel
{
  class cSettingsDialog : public win32mm::cDialog
  {
  public:
    explicit cSettingsDialog(cSettings& settings);

    bool Run(win32mm::cWindow& parent);

  private:
    virtual void OnInit() override;
    virtual bool OnCommand(int iCommand) override;
    virtual bool OnOk() override;

    void LayoutAndSetWindowSize();
    void EnableControls();

    cSettings& settings;

    win32mm::cGroupBox groupHistory;
    win32mm::cStatic historyStaticClearFolders;
    win32mm::cButton buttonHistoryClearFolders;
    win32mm::cStatic historyStaticClearCache;
    win32mm::cButton buttonHistoryClearCache;

    win32mm::cStatic historyStaticCacheSize;
    win32mm::cInputUpDown historyCacheMaximumSizeGB;
    win32mm::cStatic historyStaticGB;

    win32mm::cHorizontalLine horizontalLine;
    win32mm::cButton buttonCancel;
    win32mm::cButton buttonOk;
  };

  cSettingsDialog::cSettingsDialog(cSettings& _settings) :
    settings(_settings)
  {
  }

  void cSettingsDialog::OnInit()
  {
    int iDPI = GetDPI();
    LOG<<"DPI="<<iDPI<<std::endl;

    // Add controls
    groupHistory.Create(*this, TEXT("History"));

    historyStaticClearFolders.Create(*this, TEXT("Clear Previous Folders:"));
    buttonHistoryClearFolders.Create(*this, 102, TEXT("Clear"));
    historyStaticClearCache.Create(*this, TEXT("Clear Cache:"));
    buttonHistoryClearCache.Create(*this, 103, TEXT("Clear"));

    historyStaticCacheSize.Create(*this, TEXT("Cache Size:"));
    historyCacheMaximumSizeGB.Create(*this, 105, 1, 20, 1);
    historyCacheMaximumSizeGB.SetValue(int(settings.GetMaximumCacheSizeGB()));
    historyStaticGB.Create(*this, TEXT("GB"));

    // Add horizontal line
    horizontalLine.Create(*this);

    // Add standard buttons
    buttonOk.CreateOk(*this, TEXT("Ok"));
    buttonCancel.CreateCancel(*this);

    LayoutAndSetWindowSize();

    // Make sure our controls start in the correct state
    EnableControls();
  }

  void cSettingsDialog::LayoutAndSetWindowSize()
  {
    int iX = GetMarginWidth();
    int iY = GetMarginHeight();

    //win32mm::cGroupBox groupHistory;

    // Find the widest static text
    const int widestStaticText = max(max(MeasureStaticTextWidth(historyStaticClearFolders.GetHandle()), MeasureStaticTextWidth(historyStaticClearCache.GetHandle())), MeasureStaticTextWidth(historyStaticCacheSize.GetHandle()));

    // Find the widest row of controls on the right
    const int widthRowClearFolders = MeasureButtonWidth(historyStaticClearFolders.GetHandle());
    const int widthRowClearCache = MeasureButtonWidth(buttonHistoryClearCache.GetHandle());
    const int widthRowCacheMaximumSize = MeasureInputUpDownWidth(historyCacheMaximumSizeGB) + GetSpacerWidth() + MeasureStaticTextWidth(historyStaticGB.GetHandle());
    const int widestRowOnTheRight = max(max(widthRowClearFolders, widthRowClearCache), widthRowCacheMaximumSize);

    MoveControlStaticNextToOtherControls(historyStaticClearFolders.GetHandle(), iX, iY, widestStaticText);
    MoveControl(buttonHistoryClearFolders.GetHandle(), iX + widestStaticText + GetSpacerWidth(), iY, widestRowOnTheRight, GetButtonHeight());
    iY += GetButtonHeight() + GetSpacerHeight();

    MoveControlStaticNextToOtherControls(historyStaticClearCache.GetHandle(), iX, iY, widestStaticText);
    MoveControl(buttonHistoryClearCache.GetHandle(), iX + widestStaticText + GetSpacerWidth(), iY, widestRowOnTheRight, GetButtonHeight());
    iY += GetButtonHeight() + GetSpacerHeight();

    MoveControlStaticNextToOtherControls(historyStaticCacheSize.GetHandle(), iX, iY, widestStaticText);
    MoveControlInputUpDown(historyCacheMaximumSizeGB, iX + widestStaticText + GetSpacerWidth(), iY, widestRowOnTheRight);
    MoveControl(historyStaticGB.GetHandle(), iX + widestStaticText + GetSpacerWidth() + MeasureInputUpDownWidth(historyCacheMaximumSizeGB) + GetSpacerWidth(), iY, MeasureStaticTextWidth(historyStaticGB.GetHandle()), GetTextHeight());
    iY += GetInputHeight() + GetSpacerHeight();

    const int iWidth = widestStaticText + GetSpacerWidth() + widestRowOnTheRight;

    MoveControl(horizontalLine.GetHandle(), iX, iY, iWidth, 1);
    iY += 1 + GetSpacerHeight();

    const int iDialogWidth = GetMarginWidth() + iWidth + GetMarginWidth();
    const int iDialogHeight = iY + GetButtonHeight() + GetMarginHeight();

    MoveOkCancelHelp(iDialogWidth, iDialogHeight);

    // Set our window size now that we know how big it should be
    SetClientSize(iDialogWidth, iDialogHeight);
  }

  bool cSettingsDialog::OnCommand(int iCommand)
  {
    (void)iCommand;

    return false;
  }

  bool cSettingsDialog::OnOk()
  {
    int iCacheMaximumSizeGB = historyCacheMaximumSizeGB.GetValue();
    if ((iCacheMaximumSizeGB < 1) || (iCacheMaximumSizeGB > 20)) return false;

    settings.SetMaximumCacheSizeGB(iCacheMaximumSizeGB);

    return true;
  }

  void cSettingsDialog::EnableControls()
  {
  }

  bool cSettingsDialog::Run(win32mm::cWindow& parent)
  {
    return RunNonResizable(parent);
  }


  // ** OpenSettingsDialog

  bool OpenSettingsDialog(cSettings& settings, win32mm::cWindow& parent)
  {
    cSettingsDialog dialog(settings);
    return dialog.Run(parent);
  }
}
