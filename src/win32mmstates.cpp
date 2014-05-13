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

// OpenGL headers
#include <GL/GLee.h>
#include <GL/glu.h>

// SDL headers
#include <SDL2/SDL_image.h>

// libopenglmm headers
#include <libopenglmm/cGeometry.h>

// libwin32mm headers
#include <libwin32mm/controls.h>
#include <libwin32mm/dialog.h>
#include <libwin32mm/filebrowse.h>
#include <libwin32mm/progressdialog.h>

// Spitfire headers
#include <spitfire/spitfire.h>

#include <spitfire/math/math.h>
#include <spitfire/math/cVec2.h>
#include <spitfire/math/cVec3.h>
#include <spitfire/math/cVec4.h>
#include <spitfire/math/cMat4.h>
#include <spitfire/math/cQuaternion.h>
#include <spitfire/math/cColour.h>

#include <spitfire/storage/filesystem.h>
#include <spitfire/storage/xml.h>

#include <spitfire/util/log.h>

// Diesel headers
#include "importthread.h"
#include "win32mmapplication.h"
#include "win32mmstates.h"

namespace diesel
{
  class cSettingsDialog : public win32mm::cDialog
  {
  public:
    explicit cSettingsDialog(cSettings& settings);

    bool Run(win32mm::cWindow& parent);

  private:
    virtual void OnInit() override;
    virtual void OnCommand(int iCommand) override;
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

  void cSettingsDialog::OnCommand(int iCommand)
  {
    (void)iCommand;
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


  // ** cState

  cState::cState(cApplication& _application) :
    breathe::util::cState(_application),
    application(_application),
    settings(application.settings),
    pFont(application.pFont),
    pAudioManager(application.pAudioManager),
    pGuiManager(application.pGuiManager),
    pGuiRenderer(application.pGuiRenderer),
    pLayer(nullptr),
    bIsWireframe(false),
    pVertexBufferObjectLetterBoxedRectangle(nullptr),
    pFrameBufferObjectLetterBoxedRectangle(nullptr),
    pShaderLetterBoxedRectangle(nullptr)
  {
    breathe::gui::cLayer* pRoot = static_cast<breathe::gui::cLayer*>(pGuiManager->GetRoot());

    pLayer = new breathe::gui::cLayer;
    pRoot->AddChild(pLayer);
  }

  cState::~cState()
  {
    if (pVertexBufferObjectLetterBoxedRectangle != nullptr) DestroyVertexBufferObjectLetterBoxedRectangle();
    if (pFrameBufferObjectLetterBoxedRectangle != nullptr) DestroyFrameBufferObjectLetterBoxedRectangle();
    if (pShaderLetterBoxedRectangle != nullptr) DestroyShaderLetterBoxedRectangle();

    if (pLayer != nullptr) {
      breathe::gui::cWidget* pRoot = pGuiManager->GetRoot();
      ASSERT(pRoot != nullptr);
      pRoot->RemoveChildAndDestroy(pLayer);
    }
  }

  void cState::EnterVirtualChildDialog()
  {
    application.window.DisableMenu();
  }

  void cState::ExitVirtualChildDialog()
  {
    application.window.EnableMenu();
  }

  void cState::_OnPause()
  {
    //if (pLayer != nullptr) pLayer->SetVisible(false);
  }

  void cState::_OnResume()
  {
    if (pLayer != nullptr) {
      //pLayer->SetVisible(true);

      pLayer->SetFocusToFirstChild();
    }
  }

  LRESULT cState::_HandleWin32Event(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
  {
    return _HandleStateWin32Event(hwnd, uMsg, wParam, lParam);
  }

  void cState::_OnWindowEvent(const breathe::gui::cWindowEvent& event)
  {
    LOG<<"cState::_OnWindowEvent"<<std::endl;
    if (event.IsResized()) _OnStateResizeEvent();
    else if (event.IsQuit()) _OnStateQuitEvent();
  }

  void cState::_OnKeyboardEvent(const breathe::gui::cKeyboardEvent& event)
  {
    bool bIsHandled = false;
    if (event.IsKeyDown()) bIsHandled = pGuiManager->InjectEventKeyboardDown(event.GetKeyCode());
    else bIsHandled = pGuiManager->InjectEventKeyboardUp(event.GetKeyCode());

    if (!bIsHandled) _OnStateKeyboardEvent(event);
  }

  void cState::_OnMouseEvent(const breathe::gui::cMouseEvent& event)
  {
    const float x = event.GetX() / pContext->GetWidth();
    const float y = event.GetY() / pContext->GetHeight();

    bool bIsHandled = false;
    if (event.IsButtonDown()) bIsHandled = pGuiManager->InjectEventMouseDown(event.GetButton(), x, y);
    else if (event.IsButtonUp()) bIsHandled = pGuiManager->InjectEventMouseUp(event.GetButton(), x, y);
    else bIsHandled = pGuiManager->InjectEventMouseMove(event.GetButton(), x, y);

    if (!bIsHandled) _OnStateMouseEvent(event);
  }

  breathe::gui::cStaticText* cState::AddStaticText(breathe::gui::id_t id, const spitfire::string_t& sText, float x, float y, float width)
  {
    breathe::gui::cStaticText* pStaticText = new breathe::gui::cStaticText;
    pStaticText->SetId(id);
    pStaticText->SetCaption(sText);
    pStaticText->SetRelativePosition(spitfire::math::cVec2(x, y));
    pStaticText->SetWidth(width);
    pStaticText->SetHeight(pGuiManager->GetStaticTextHeight());
    pLayer->AddChild(pStaticText);

    return pStaticText;
  }

  breathe::gui::cRetroButton* cState::AddRetroButton(breathe::gui::id_t id, const spitfire::string_t& sText, float x, float y, float width)
  {
    breathe::gui::cRetroButton* pRetroButton = new breathe::gui::cRetroButton;
    pRetroButton->SetEventListener(*this);
    pRetroButton->SetId(id);
    pRetroButton->SetCaption(sText);
    pRetroButton->SetRelativePosition(spitfire::math::cVec2(x, y));
    pRetroButton->SetWidth(width);
    pRetroButton->SetHeight(pGuiManager->GetStaticTextHeight());
    pLayer->AddChild(pRetroButton);

    return pRetroButton;
  }

  breathe::gui::cRetroInput* cState::AddRetroInput(breathe::gui::id_t id, const spitfire::string_t& sText, float x, float y, float width)
  {
    breathe::gui::cRetroInput* pRetroInput = new breathe::gui::cRetroInput;
    pRetroInput->SetEventListener(*this);
    pRetroInput->SetId(id);
    pRetroInput->SetCaption(sText);
    pRetroInput->SetRelativePosition(spitfire::math::cVec2(x, y));
    pRetroInput->SetWidth(width);
    pRetroInput->SetHeight(pGuiManager->GetStaticTextHeight());
    pLayer->AddChild(pRetroInput);

    return pRetroInput;
  }

  breathe::gui::cRetroInputUpDown* cState::AddRetroInputUpDown(breathe::gui::id_t id, int min, int max, int value, float x, float y, float width)
  {
    breathe::gui::cRetroInputUpDown* pRetroInputUpDown = new breathe::gui::cRetroInputUpDown;
    pRetroInputUpDown->SetEventListener(*this);
    pRetroInputUpDown->SetId(id);
    pRetroInputUpDown->SetRange(min, max);
    pRetroInputUpDown->SetValue(value, false);
    pRetroInputUpDown->SetRelativePosition(spitfire::math::cVec2(x, y));
    pRetroInputUpDown->SetWidth(width);
    pRetroInputUpDown->SetHeight(pGuiManager->GetStaticTextHeight());
    pLayer->AddChild(pRetroInputUpDown);

    return pRetroInputUpDown;
  }

  breathe::gui::cRetroColourPicker* cState::AddRetroColourPicker(breathe::gui::id_t id, float x, float y, float width)
  {
    breathe::gui::cRetroColourPicker* pRetroColourPicker = new breathe::gui::cRetroColourPicker;
    pRetroColourPicker->SetEventListener(*this);
    pRetroColourPicker->SetId(id);
    pRetroColourPicker->SetRelativePosition(spitfire::math::cVec2(x, y));
    pRetroColourPicker->SetWidth(width);
    pRetroColourPicker->SetHeight(pGuiManager->GetStaticTextHeight());
    pLayer->AddChild(pRetroColourPicker);

    return pRetroColourPicker;
  }

  void cState::CreateVertexBufferObjectLetterBoxedRectangle(size_t width, size_t height)
  {
    ASSERT(pVertexBufferObjectLetterBoxedRectangle == nullptr);

    pVertexBufferObjectLetterBoxedRectangle = pContext->CreateStaticVertexBufferObject();

    opengl::cGeometryDataPtr pGeometryDataPtr = opengl::CreateGeometryData();

    opengl::cGeometryBuilder_v2_c4_t2 builder(*pGeometryDataPtr);


    cLetterBox letterBox(width, height);

    const float fWidth = float(letterBox.letterBoxedWidth);
    const float fHeight = float(letterBox.letterBoxedHeight);

    // Texture coordinates
    // NOTE: The v coordinates have been swapped, the code looks correct but with normal v coordinates the gui is rendered upside down
    const float fU = 0.0f;
    const float fV = float(letterBox.letterBoxedHeight);
    const float fU2 = float(letterBox.letterBoxedWidth);
    const float fV2 = 0.0f;

    const float x = 0.0f;
    const float y = 0.0f;

    const spitfire::math::cColour colour(1.0f, 1.0f, 1.0f, 1.0f);

    // Front facing triangles
    builder.PushBack(spitfire::math::cVec2(x, y + fHeight), colour, spitfire::math::cVec2(fU, fV2));
    builder.PushBack(spitfire::math::cVec2(x + fWidth, y + fHeight), colour, spitfire::math::cVec2(fU2, fV2));
    builder.PushBack(spitfire::math::cVec2(x + fWidth, y), colour, spitfire::math::cVec2(fU2, fV));
    builder.PushBack(spitfire::math::cVec2(x + fWidth, y), colour, spitfire::math::cVec2(fU2, fV));
    builder.PushBack(spitfire::math::cVec2(x, y), colour, spitfire::math::cVec2(fU, fV));
    builder.PushBack(spitfire::math::cVec2(x, y + fHeight), colour, spitfire::math::cVec2(fU, fV2));

    pVertexBufferObjectLetterBoxedRectangle->SetData(pGeometryDataPtr);

    pVertexBufferObjectLetterBoxedRectangle->Compile2D(system);
  }

  void cState::DestroyVertexBufferObjectLetterBoxedRectangle()
  {
    if (pVertexBufferObjectLetterBoxedRectangle != nullptr) {
      pContext->DestroyStaticVertexBufferObject(pVertexBufferObjectLetterBoxedRectangle);
      pVertexBufferObjectLetterBoxedRectangle = nullptr;
    }
  }

  void cState::CreateFrameBufferObjectLetterBoxedRectangle(size_t width, size_t height)
  {
    ASSERT(pFrameBufferObjectLetterBoxedRectangle == nullptr);

    cLetterBox letterBox(width, height);

    pFrameBufferObjectLetterBoxedRectangle = pContext->CreateTextureFrameBufferObjectNoMipMaps(letterBox.letterBoxedWidth, letterBox.letterBoxedHeight, opengl::PIXELFORMAT::R8G8B8A8);
  }

  void cState::DestroyFrameBufferObjectLetterBoxedRectangle()
  {
    if (pFrameBufferObjectLetterBoxedRectangle != nullptr) {
      pContext->DestroyTextureFrameBufferObject(pFrameBufferObjectLetterBoxedRectangle);
      pFrameBufferObjectLetterBoxedRectangle = nullptr;
    }
  }

  void cState::CreateShaderLetterBoxedRectangle()
  {
    ASSERT(pShaderLetterBoxedRectangle == nullptr);

    pShaderLetterBoxedRectangle = pContext->CreateShader(TEXT("data/shaders/passthroughwithcolour.vert"), TEXT("data/shaders/passthroughwithcolourrect.frag"));
  }

  void cState::DestroyShaderLetterBoxedRectangle()
  {
    if (pShaderLetterBoxedRectangle != nullptr) {
      pContext->DestroyShader(pShaderLetterBoxedRectangle);
      pShaderLetterBoxedRectangle = nullptr;
    }
  }

  void cState::LoadResources()
  {
    const size_t width = pContext->GetWidth();
    const size_t height = pContext->GetHeight();
    CreateFrameBufferObjectLetterBoxedRectangle(width, height);
    CreateShaderLetterBoxedRectangle();
    CreateVertexBufferObjectLetterBoxedRectangle(width, height);
  }

  void cState::DestroyResources()
  {
    DestroyFrameBufferObjectLetterBoxedRectangle();
    DestroyShaderLetterBoxedRectangle();
    DestroyVertexBufferObjectLetterBoxedRectangle();
  }

  void cState::_Render(const spitfire::math::cTimeStep& timeStep)
  {
    const size_t width = pContext->GetWidth();
    const size_t height = pContext->GetHeight();

    cLetterBox letterBox(width, height);

    if ((width == letterBox.desiredWidth) || (height == letterBox.desiredHeight)) {
      // Render the scene
      const spitfire::math::cColour clearColour(0.392156863f, 0.584313725f, 0.929411765f);
      pContext->SetClearColour(clearColour);

      pContext->BeginRenderToScreen();

        if (bIsWireframe) pContext->EnableWireframe();

        _RenderToTexture(timeStep);

      pContext->EndRenderToScreen(*pWindow);
    } else {
      // Render the scene to a texture and draw the texture to the screen letter boxed

      if (pVertexBufferObjectLetterBoxedRectangle == nullptr) CreateVertexBufferObjectLetterBoxedRectangle(width, height);
      if (pFrameBufferObjectLetterBoxedRectangle == nullptr) CreateFrameBufferObjectLetterBoxedRectangle(width, height);
      if (pShaderLetterBoxedRectangle == nullptr) CreateShaderLetterBoxedRectangle();

      // Render the scene to the texture
      {
        const spitfire::math::cColour clearColour(0.392156863f, 0.584313725f, 0.929411765f);
        pContext->SetClearColour(clearColour);

        pContext->BeginRenderToTexture(*pFrameBufferObjectLetterBoxedRectangle);

          if (bIsWireframe) pContext->EnableWireframe();

          _RenderToTexture(timeStep);

        pContext->EndRenderToTexture(*pFrameBufferObjectLetterBoxedRectangle);
      }

      // Render the texture to the screen
      {
        const spitfire::math::cColour clearColour(1.0f, 0.0f, 0.0f);
        pContext->SetClearColour(clearColour);

        pContext->BeginRenderToScreen();

          //if (bIsWireframe) pContext->EnableWireframe();

          pContext->BeginRenderMode2D(opengl::MODE2D_TYPE::Y_INCREASES_DOWN_SCREEN);

            {
              // Set the position of the layer
              spitfire::math::cMat4 matModelView2D;
              if (letterBox.fRatio < letterBox.fDesiredRatio) matModelView2D.SetTranslation(0.0f, float((height - letterBox.letterBoxedHeight) / 2), 0.0f);
              else matModelView2D.SetTranslation(float((width - letterBox.letterBoxedWidth) / 2), 0.0f, 0.0f);

              pContext->EnableBlending();

              pContext->BindTexture(0, *pFrameBufferObjectLetterBoxedRectangle);

              pContext->BindShader(*pShaderLetterBoxedRectangle);

              pContext->SetShaderProjectionAndModelViewMatricesRenderMode2D(opengl::MODE2D_TYPE::Y_INCREASES_DOWN_SCREEN_KEEP_DIMENSIONS_AND_ASPECT_RATIO, matModelView2D);

              pContext->BindStaticVertexBufferObject2D(*pVertexBufferObjectLetterBoxedRectangle);
              pContext->DrawStaticVertexBufferObjectTriangles2D(*pVertexBufferObjectLetterBoxedRectangle);
              pContext->UnBindStaticVertexBufferObject2D(*pVertexBufferObjectLetterBoxedRectangle);

              pContext->UnBindShader(*pShaderLetterBoxedRectangle);

              pContext->UnBindTexture(0, *pFrameBufferObjectLetterBoxedRectangle);

              pContext->DisableBlending();
            }

          pContext->EndRenderMode2D();

        pContext->EndRenderToScreen(*pWindow);
      }
    }
  }


  // ** cStatePhotoBrowser

  const int ID_CONTROL_PATH = 101;
  const int ID_CONTROL_UP = 102;
  const int ID_CONTROL_SHOW_FOLDER = 103;
  const int ID_CONTROL_SCROLLBAR = 104;
  }

  namespace diesel
  {
  cStatePhotoBrowser::cStatePhotoBrowser(cApplication& application) :
    cState(application),
    bIsKeyReturn(false),
    bIsSinglePhotoMode(false)
  {
    LOG<<"cStatePhotoBrowser::cStatePhotoBrowser"<<std::endl;

    application.window.InitWindowProc();

    comboBoxPath.CreateComboBox(application.window, ID_CONTROL_PATH);
    comboBoxPath.AddString(TEXT("a"));
    comboBoxPath.AddString(TEXT("c"));
    comboBoxPath.AddString(TEXT("b"));

    comboBoxPath.SetText(TEXT("Hello"));

    win32mm::cIcon iconUp;
    iconUp.LoadFromFile(TEXT("data/icons/windows/folder_up.ico"), 32);

    buttonPathUp.Create(application.window, ID_CONTROL_UP, iconUp);

    win32mm::cIcon iconShowFolder;
    iconShowFolder.LoadFromFile(TEXT("data/icons/windows/folder_show.ico"), 32);

    buttonPathShowFolder.Create(application.window, ID_CONTROL_SHOW_FOLDER, iconShowFolder);

    scrollBar.CreateVertical(application.window, 101);
    scrollBar.SetRange(0, 200);
    scrollBar.SetPageSize(20);
    scrollBar.SetPosition(50);

    const float fSpacerWidth = pGuiManager->GetSpacerWidth();
    const float fSpacerHeight = pGuiManager->GetSpacerHeight();

    const float fLayerWidth = pLayer->GetWidth();

    breathe::gui::cToolbar* pToolbar = new breathe::gui::cToolbar;
    pToolbar->SetId(106);
    pToolbar->SetCaption(TEXT("Toolbar"));
    pToolbar->SetRelativePosition(spitfire::math::cVec2(fLayerWidth - pGuiManager->GetToolbarWidthOrHeight(), 0.0f));
    pToolbar->SetWidth(pGuiManager->GetToolbarWidthOrHeight());
    pToolbar->SetHeight(pLayer->GetHeight());
    pLayer->AddChild(pToolbar);

    breathe::gui::cScrollbar* pScrollbar = new breathe::gui::cScrollbar;
    pScrollbar->SetId(107);
    pScrollbar->SetCaption(TEXT("Scrollbar"));
    pToolbar->SetRelativePosition(spitfire::math::cVec2(fLayerWidth- (pGuiManager->GetToolbarWidthOrHeight() + pGuiManager->GetScrollBarWidthOrHeight()), 0.0f));
    pToolbar->SetWidth(pGuiManager->GetScrollBarWidthOrHeight());
    pToolbar->SetHeight(pLayer->GetHeight());
    pLayer->AddChild(pScrollbar);

    breathe::gui::cStaticText* pStaticText = new breathe::gui::cStaticText;
    pStaticText->SetId(102);
    pStaticText->SetCaption(TEXT("StaticText"));
    pStaticText->SetRelativePosition(spitfire::math::cVec2(fSpacerWidth, fSpacerHeight));
    pStaticText->SetWidth(0.15f);
    pStaticText->SetHeight(pGuiManager->GetStaticTextHeight());
    pLayer->AddChild(pStaticText);

    breathe::gui::cButton* pButton = new breathe::gui::cButton;
    pButton->SetId(103);
    pButton->SetCaption(TEXT("Button"));
    pButton->SetRelativePosition(spitfire::math::cVec2(pStaticText->GetX() + pStaticText->GetWidth() + fSpacerWidth, fSpacerHeight));
    pButton->SetWidth(0.15f);
    pButton->SetHeight(pGuiManager->GetButtonHeight());
    pLayer->AddChild(pButton);

    breathe::gui::cComboBox* pComboBox = new breathe::gui::cComboBox;
    pComboBox->SetId(104);
    pComboBox->SetCaption(TEXT("ComboBox"));
    pComboBox->SetRelativePosition(spitfire::math::cVec2(pStaticText->GetX(), pStaticText->GetY() + pStaticText->GetHeight() + fSpacerHeight));
    pComboBox->SetWidth(0.15f);
    pComboBox->SetHeight(pGuiManager->GetComboBoxHeight());
    pLayer->AddChild(pComboBox);

    breathe::gui::cSlider* pSlider = new breathe::gui::cSlider;
    pSlider->SetId(105);
    pSlider->SetCaption(TEXT("Slider"));
    pSlider->SetRelativePosition(spitfire::math::cVec2(pStaticText->GetX() + pStaticText->GetWidth() + fSpacerWidth, pStaticText->GetY() + pStaticText->GetHeight() + fSpacerHeight));
    pSlider->SetWidth(0.15f);
    pSlider->SetHeight(0.1f);
    pLayer->AddChild(pSlider);

    pLayer->SetFocusToNextChild();

    _OnStateResizeEvent();
  }

  cStatePhotoBrowser::~cStatePhotoBrowser()
  {
    comboBoxPath.Destroy();
    scrollBar.Destroy();

    application.window.DestroyWindowProc();
  }

  void cStatePhotoBrowser::_Update(const spitfire::math::cTimeStep& timeStep)
  {
    pGuiRenderer->Update();
  }
  
  LRESULT cStatePhotoBrowser::_HandleStateWin32Event(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
  {
    switch (uMsg) {
      case WM_COMMAND: {
        // The user used a control or selected a menu item
        const int iCommandID = (LOWORD(wParam));

        // Handle the command
        HandleCommand(iCommandID);

        break;
      }
      default: {
        //
        break;
      }
    }

    return FALSE;
  }

  void cStatePhotoBrowser::HandleCommand(int iCommandID)
  {
    LOG<<"cStatePhotoBrowser::HandleCommand id="<<iCommandID<<std::endl;

    if (iCommandID == ID_MENU_FILE_OPEN_FOLDER) {
      win32mm::cWin32mmFolderDialog dialog;
      dialog.Run(application.window);
    } else if (iCommandID == ID_MENU_FILE_SETTINGS) {
      cSettingsDialog dialog(settings);
      dialog.Run(application.window);
    } else if (iCommandID == ID_MENU_FILE_QUIT) {
      _OnStateQuitEvent();
    } else if (iCommandID == ID_MENU_EDIT_CUT) {

    } else if (iCommandID == ID_MENU_VIEW_SINGLE_PHOTO_MODE) {
      bIsSinglePhotoMode = !bIsSinglePhotoMode;

      // TODO: UPDATE THE MENU ITEM CHECKBOX
    } else if (iCommandID == ID_CONTROL_PATH) {
      LOG<<"cStatePhotoBrowser::HandleCommand Path"<<std::endl;
    } else if (iCommandID == ID_CONTROL_UP) {
      LOG<<"cStatePhotoBrowser::HandleCommand Up"<<std::endl;
    }
  }

  void cStatePhotoBrowser::_OnStateQuitEvent()
  {
    LOG<<"cStatePhotoBrowser::_OnStateQuitEvent"<<std::endl;
    // Pop our menu state
    application.PopStateSoon();
  }

  void cStatePhotoBrowser::_OnStateMouseEvent(const breathe::gui::cMouseEvent& event)
  {
    if (event.IsButtonUp() && (event.GetButton() == 3)) {
      win32mm::cPopupMenu popupMenu;
      popupMenu.AppendMenuItem(12345, TEXT("a"));
      popupMenu.AppendMenuItem(12346, TEXT("b"));
      popupMenu.AppendMenuItem(12347, TEXT("c"));

      application.window.DisplayPopupMenu(popupMenu);
    }
  }

  void cStatePhotoBrowser::_OnStateKeyboardEvent(const breathe::gui::cKeyboardEvent& event)
  {
    if (event.IsKeyUp()) {
      switch (event.GetKeyCode()) {
        case breathe::gui::KEY::NUMBER_1: {
          LOG<<"cStatePhotoBrowser::_OnStateKeyboardEvent 1 up"<<std::endl;
          bIsWireframe = !bIsWireframe;
          break;
        }
        case breathe::gui::KEY::NUMBER_2: {
          LOG<<"cStatePhotoBrowser::_OnStateKeyboardEvent 2 up"<<std::endl;
          break;
        }
        case breathe::gui::KEY::NUMBER_3: {
          EnterVirtualChildDialog();
          break;
        }
        case breathe::gui::KEY::NUMBER_4: {
          ExitVirtualChildDialog();
          break;
        }
      }
    }
  }

  breathe::gui::EVENT_RESULT cStatePhotoBrowser::_OnWidgetEvent(const breathe::gui::cWidgetEvent& event)
  {
    LOG<<"cStatePhotoBrowser::_OnWidgetEvent"<<std::endl;

    if (event.IsPressed()) {
      switch (event.GetWidget()->GetId()) {
        case OPTION::NEW_GAME: {
          // Push our game state
          application.PushStateSoon(new cStateAboutBox(application));
          break;
        }
        case OPTION::QUIT: {
          // Pop our menu state
          application.PopStateSoon();
          break;
        }
      }
    }

    return breathe::gui::EVENT_RESULT::NOT_HANDLED_PERCOLATE;
  }

  void cStatePhotoBrowser::_RenderToTexture(const spitfire::math::cTimeStep& timeStep)
  {
    // Render the scene
    const spitfire::math::cColour clearColour(0.392156863f, 0.584313725f, 0.929411765f);
    pContext->SetClearColour(clearColour);

    {
      if (pGuiRenderer != nullptr) {
        pGuiRenderer->SetWireFrame(bIsWireframe);
        pGuiRenderer->Render();
      }
    }
  }

  void cStatePhotoBrowser::_OnStateResizeEvent()
  {
    int iWindowWidth = 0;
    int iWindowHeight = 0;
    application.window.GetClientSize(iWindowWidth, iWindowHeight);

    int iStatusBarWidth = 0;
    int iStatusBarHeight = 0;
    application.window.GetControlSize(application.statusBar.GetHandle(), iStatusBarWidth, iStatusBarHeight);

    const int iHeight = application.window.DialogUnitsToPixelsY(34);
    // TODO: Why doesn't this work?
    //const int iComboBoxHeight = application.window.GetComboBoxHeight();
    const int iComboBoxHeight = application.window.DialogUnitsToPixelsY(22);
    const int iButtonWidth = iHeight;
    const int iButtonsTotalWidth = (2 * (iButtonWidth + application.window.GetSpacerWidth()));
    const int iScrollBarWidth = application.window.GetScrollBarWidth();
    const int iScrollBarHeight = iWindowHeight - iStatusBarHeight;
    const int iComboBoxWidth = iWindowWidth - (iButtonsTotalWidth + (2 * application.window.GetSpacerWidth()) + iScrollBarWidth);

    int x = application.window.GetSpacerWidth();
    const int y = (iHeight / 2) - (iComboBoxHeight / 2);
    application.window.MoveControl(comboBoxPath.GetHandle(), x, y, iComboBoxWidth, iComboBoxHeight);
    x += iComboBoxWidth + application.window.GetSpacerWidth();

    application.window.MoveControl(buttonPathUp.GetHandle(), x, 0, iButtonWidth, iHeight);
    x += iButtonWidth + application.window.GetSpacerWidth();
    application.window.MoveControl(buttonPathShowFolder.GetHandle(), x, 0, iButtonWidth, iHeight);
    x += iButtonWidth + application.window.GetSpacerWidth();

    application.window.MoveControl(scrollBar.GetHandle(), x, 0, iScrollBarWidth, iScrollBarHeight);

    scrollBar.SetRange(0, 200);
    scrollBar.SetPosition(50);
  }


  // ** cStateAboutBox

  cStateAboutBox::cStateAboutBox(cApplication& application) :
    cState(application),

    pTextureBlock(nullptr),

    pShaderBlock(nullptr),

    bPauseSoon(false),
    bQuitSoon(false)
  {
    pTextureBlock = pContext->CreateTexture(TEXT("data/textures/block.png"));

    pShaderBlock = pContext->CreateShader(TEXT("data/shaders/passthroughwithcolour.vert"), TEXT("data/shaders/passthroughwithcolour.frag"));

    const spitfire::durationms_t currentTime = SDL_GetTicks();

    spitfire::math::SetRandomSeed(currentTime);


    const spitfire::math::cColour red(1.0f, 0.0f, 0.0f);
    const spitfire::math::cColour green(0.0f, 1.0f, 0.0f);
    const spitfire::math::cColour blue(0.0f, 0.0f, 1.0f);
    const spitfire::math::cColour yellow(1.0f, 1.0f, 0.0f);
  }

  cStateAboutBox::~cStateAboutBox()
  {
    LOG<<"cStateAboutBox::~cStateAboutBox"<<std::endl;

    if (pShaderBlock != nullptr) {
      pContext->DestroyShader(pShaderBlock);
      pShaderBlock = nullptr;
    }

    if (pTextureBlock != nullptr) {
      pContext->DestroyTexture(pTextureBlock);
      pTextureBlock = nullptr;
    }


    LOG<<"cStateAboutBox::~cStateAboutBox returning"<<std::endl;
  }

  void cStateAboutBox::SetQuitSoon()
  {
    bQuitSoon = true;
  }

  void cStateAboutBox::UpdateText()
  {
    /*for (size_t i = 0; i < game.boards.size(); i++) {
      tetris::cBoard& board = *(game.boards[i]);

      spitfire::ostringstream_t o;

      const uint32_t uiLevel = board.GetLevel();
      o<<TEXT("Level ");
      o<<uiLevel;
      pLevelText[i]->SetCaption(o.str());
      o.str(TEXT(""));

      const uint32_t uiScore = board.GetScore();
      o<<TEXT("Score ");
      o<<uiScore;
      pScoreText[i]->SetCaption(o.str());
      o.str(TEXT(""));
    }*/
  }

  void cStateAboutBox::_OnStateKeyboardEvent(const breathe::gui::cKeyboardEvent& event)
  {
    LOG<<"cStateAboutBox::_OnStateKeyboardEvent"<<std::endl;

    if (event.IsKeyDown()) {
      LOG<<"cStateAboutBox::_OnStateKeyboardEvent Key down"<<std::endl;
      switch (event.GetKeyCode()) {
        case breathe::gui::KEY::ESCAPE: {
          LOG<<"cStateAboutBox::_OnStateKeyboardEvent Escape down"<<std::endl;
          bPauseSoon = true;
          break;
        }
      }
    } else if (event.IsKeyUp()) {
      switch (event.GetKeyCode()) {
        case breathe::gui::KEY::NUMBER_1: {
          LOG<<"cStateAboutBox::_OnStateKeyboardEvent 1 up"<<std::endl;
          bIsWireframe = !bIsWireframe;
          break;
        }
      }
    }
  }

  void cStateAboutBox::_Update(const spitfire::math::cTimeStep& timeStep)
  {
    if (bQuitSoon) {
      // Pop our menu state
      application.PopStateSoon();
    }

    pGuiRenderer->Update();
  }

  void cStateAboutBox::_UpdateInput(const spitfire::math::cTimeStep& timeStep)
  {
    assert(pWindow != nullptr);


  }

  void cStateAboutBox::_RenderToTexture(const spitfire::math::cTimeStep& timeStep)
  {
    // Render the scene
    const spitfire::math::cColour clearColour(0.392156863f, 0.584313725f, 0.929411765f);
    pContext->SetClearColour(clearColour);

    if (bIsWireframe) pContext->EnableWireframe();

    {
      pContext->BeginRenderMode2D(breathe::render::MODE2D_TYPE::Y_INCREASES_DOWN_SCREEN_KEEP_ASPECT_RATIO);

      pContext->EndRenderMode2D();

      if (pGuiRenderer != nullptr) {
        pGuiRenderer->SetWireFrame(bIsWireframe);
        pGuiRenderer->Render();
      }
    }
  }


  //pFont->PrintCenteredHorizontally(x, y, 1.0f, breathe::LANG("L_Paused").c_str());
  //pFont->PrintCenteredHorizontally(x, y + 0.05f, 1.0f, breathe::LANG("L_Paused_Instructions").c_str());

  //pFont->PrintCenteredHorizontally(x, y, 1.0f, breathe::LANG("L_GameOver").c_str());
  //pFont->PrintCenteredHorizontally(x, y + 0.05f, 1.0f, breathe::LANG("L_GameOver_Instructions1").c_str());
  //pFont->PrintCenteredHorizontally(x, y + 0.10f, 1.0f, breathe::LANG("L_GameOver_Instructions2").c_str());
}
