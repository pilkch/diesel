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

#ifdef __WIN__
// libwin32mm headers
#include <libwin32mm/filebrowse.h>
#include <libwin32mm/window.h>
#endif

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
#include "win32mmapplication.h"
#include "win32mmstates.h"

namespace diesel
{
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

  void cState::_OnPause()
  {
    if (pLayer != nullptr) pLayer->SetVisible(false);
  }

  void cState::_OnResume()
  {
    if (pLayer != nullptr) {
      pLayer->SetVisible(true);

      pLayer->SetFocusToFirstChild();
    }
  }

  void cState::_OnWindowEvent(const breathe::gui::cWindowEvent& event)
  {
    LOG<<"cState::_OnWindowEvent"<<std::endl;
    if (event.IsCommand()) {
      LOG<<"cState::_OnWindowEvent Sending to state command event"<<std::endl;
      _OnStateCommandEvent(event.GetCommandID());
    }
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

  cStatePhotoBrowser::cStatePhotoBrowser(cApplication& application) :
    cState(application),
    bIsKeyReturn(false),
    bIsSinglePhotoMode(false)
  {
    std::cout<<"cStatePhotoBrowser::cStatePhotoBrowser"<<std::endl;

    const breathe::gui::id_t ids[] = {
      OPTION::NEW_GAME,
      OPTION::HIGH_SCORES,
      //OPTION::PREFERENCES,
      OPTION::QUIT,
    };
    const spitfire::string_t options[] = {
      TEXT("New Game"),
      TEXT("High Scores"),
      //TEXT("Preferences"),
      TEXT("Quit")
    };

    const float x = 0.04f;
    float y = 0.2f;

    const size_t n = countof(options);
    for (size_t i = 0; i < n; i++) {
      // Create the text for this option
      AddRetroButton(ids[i], options[i], x, y, 0.15f);

      y += pGuiManager->GetStaticTextHeight() + 0.007f;
    }

    pLayer->SetFocusToNextChild();


    breathe::gui::cWindow* pWindow = new breathe::gui::cWindow;
    pWindow->SetId(101);
    pWindow->SetCaption(TEXT("Caption"));
    pWindow->SetRelativePosition(spitfire::math::cVec2(0.1f, 0.15f));
    pWindow->SetWidth(0.05f + (2.0f * (0.1f + 0.05f)));
    pWindow->SetHeight(0.05f + (2.0f * (0.1f + 0.05f)));
    pLayer->AddChild(pWindow);

    //pWindow->SetVisible(false);

    breathe::gui::cStaticText* pStaticText = new breathe::gui::cStaticText;
    pStaticText->SetId(102);
    pStaticText->SetCaption(TEXT("StaticText"));
    pStaticText->SetRelativePosition(spitfire::math::cVec2(0.03f, 0.05f));
    pStaticText->SetWidth(0.15f);
    pStaticText->SetHeight(pGuiManager->GetStaticTextHeight());
    pWindow->AddChild(pStaticText);

    breathe::gui::cButton* pButton = new breathe::gui::cButton;
    pButton->SetId(103);
    pButton->SetCaption(TEXT("Button"));
    pButton->SetRelativePosition(spitfire::math::cVec2(pStaticText->GetX() + pStaticText->GetWidth() + 0.05f, 0.05f));
    pButton->SetWidth(0.15f);
    pButton->SetHeight(pGuiManager->GetButtonHeight());
    pWindow->AddChild(pButton);

    breathe::gui::cInput* pInput = new breathe::gui::cInput;
    pInput->SetId(104);
    pInput->SetCaption(TEXT("Input"));
    pInput->SetRelativePosition(spitfire::math::cVec2(pStaticText->GetX(), pStaticText->GetY() + pStaticText->GetHeight() + 0.05f));
    pInput->SetWidth(0.15f);
    pInput->SetHeight(pGuiManager->GetInputHeight());
    pWindow->AddChild(pInput);

    breathe::gui::cSlider* pSlider = new breathe::gui::cSlider;
    pSlider->SetId(105);
    pSlider->SetCaption(TEXT("Slider"));
    pSlider->SetRelativePosition(spitfire::math::cVec2(pStaticText->GetX() + pStaticText->GetWidth() + 0.05f, pStaticText->GetY() + pStaticText->GetHeight() + 0.05f));
    pSlider->SetWidth(0.15f);
    pSlider->SetHeight(0.1f);
    pWindow->AddChild(pSlider);

    breathe::gui::cToolbar* pToolbar = new breathe::gui::cToolbar;
    pToolbar->SetId(105);
    pToolbar->SetCaption(TEXT("Toolbar"));
    pToolbar->SetRelativePosition(spitfire::math::cVec2(pSlider->GetX(), pSlider->GetY() + pSlider->GetHeight() + 0.05f));
    pToolbar->SetWidth(0.4f);
    pToolbar->SetHeight(0.1f);
    pWindow->AddChild(pToolbar);
  }

  void cStatePhotoBrowser::_Update(const spitfire::math::cTimeStep& timeStep)
  {
    pGuiRenderer->Update();
  }

  void cStatePhotoBrowser::_OnStateCommandEvent(int iCommandID)
  {
    LOG<<"cStatePhotoBrowser::_OnStateCommandEvent id="<<iCommandID<<std::endl;
    if (iCommandID == ID_MENU_FILE_OPEN_FOLDER) {
      ASSERT(pWindow != nullptr);

      // Get the windows handle
      HWND hwndWindow = pWindow->GetWindowHandle();

      win32mm::cWindow window;

      window.SetWindowHandle(hwndWindow);

      win32mm::cWin32mmFolderDialog dialog;
      dialog.Run(window);
    } else if (iCommandID == ID_MENU_FILE_QUIT) {
      // Pop our menu state
      application.PopStateSoon();
    } else if (iCommandID == ID_MENU_EDIT_CUT) {

    }
  }
  
  void cStatePhotoBrowser::_OnStateMouseEvent(const breathe::gui::cMouseEvent& event)
  {
    if (event.IsButtonUp() && (event.GetButton() == 3)) {
      // Get the windows handle
      HWND hwndWindow = pWindow->GetWindowHandle();

      win32mm::cWindow window;

      window.SetWindowHandle(hwndWindow);

      win32mm::cPopupMenu popupMenu;
      popupMenu.AppendMenuItem(12345, TEXT("a"));
      popupMenu.AppendMenuItem(12346, TEXT("b"));

      window.DisplayPopupMenu(popupMenu);
    }
  }

  void cStatePhotoBrowser::_OnStateKeyboardEvent(const breathe::gui::cKeyboardEvent& event)
  {
    if (event.IsKeyUp()) {
      switch (event.GetKeyCode()) {
        case breathe::gui::KEY::NUMBER_1: {
          std::cout<<"cStatePhotoBrowser::_OnStateKeyboardEvent 1 up"<<std::endl;
          bIsWireframe = !bIsWireframe;
          break;
        }
        case breathe::gui::KEY::NUMBER_2: {
          std::cout<<"cStatePhotoBrowser::_OnStateKeyboardEvent 2 up"<<std::endl;
          break;
        }
      }
    }
  }

  breathe::gui::EVENT_RESULT cStatePhotoBrowser::_OnWidgetEvent(const breathe::gui::cWidgetEvent& event)
  {
    std::cout<<"cStatePhotoBrowser::_OnWidgetEvent"<<std::endl;

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
    std::cout<<"cStateAboutBox::~cStateAboutBox"<<std::endl;

    if (pShaderBlock != nullptr) {
      pContext->DestroyShader(pShaderBlock);
      pShaderBlock = nullptr;
    }

    if (pTextureBlock != nullptr) {
      pContext->DestroyTexture(pTextureBlock);
      pTextureBlock = nullptr;
    }


    std::cout<<"cStateAboutBox::~cStateAboutBox returning"<<std::endl;
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
    std::cout<<"cStateAboutBox::_OnStateKeyboardEvent"<<std::endl;

    if (event.IsKeyDown()) {
      std::cout<<"cStateAboutBox::_OnStateKeyboardEvent Key down"<<std::endl;
      switch (event.GetKeyCode()) {
        case breathe::gui::KEY::ESCAPE: {
          std::cout<<"cStateAboutBox::_OnStateKeyboardEvent Escape down"<<std::endl;
          bPauseSoon = true;
          break;
        }
      }
    } else if (event.IsKeyUp()) {
      switch (event.GetKeyCode()) {
        case breathe::gui::KEY::NUMBER_1: {
          std::cout<<"cStateAboutBox::_OnStateKeyboardEvent 1 up"<<std::endl;
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
