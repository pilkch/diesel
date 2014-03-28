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

// Spitfire headers
#include <spitfire/spitfire.h>

#include <spitfire/math/math.h>
#include <spitfire/math/cVec2.h>
#include <spitfire/math/cVec3.h>
#include <spitfire/math/cVec4.h>
#include <spitfire/math/cMat4.h>
#include <spitfire/math/cQuaternion.h>
#include <spitfire/math/cColour.h>

#include <spitfire/util/log.h>

// libopenglmm headers
#include <libopenglmm/libopenglmm.h>
#include <libopenglmm/cContext.h>
#include <libopenglmm/cFont.h>
#include <libopenglmm/cGeometry.h>
#include <libopenglmm/cShader.h>
#include <libopenglmm/cSystem.h>
#include <libopenglmm/cTexture.h>
#include <libopenglmm/cVertexBufferObject.h>
#include <libopenglmm/cWindow.h>

// libwin32mm headers
#include <libwin32mm/notify.h>

// Diesel headers
#include "win32mmapplication.h"
#include "win32mmstates.h"

namespace diesel
{
  // ** cLetterBox

  cLetterBox::cLetterBox(size_t width, size_t height) :
    desiredWidth(0),
    desiredHeight(0),
    fDesiredRatio(0.0f),
    fRatio(0.0f),
    letterBoxedWidth(0),
    letterBoxedHeight(0)
  {
    desiredWidth = 1920;
    desiredHeight = 1080;
    fDesiredRatio = float(desiredWidth) / float(desiredHeight);

    fRatio = float(width) / float(height);

    // Apply letter boxing
    letterBoxedWidth = width;
    letterBoxedHeight = height;

    if (fRatio < fDesiredRatio) {
      // Taller (4:3, 16:10 for example)
      letterBoxedHeight = width / fDesiredRatio;
    } else {
      // Wider
      letterBoxedWidth = height * fDesiredRatio;
    }

    // Round up to the next even number
    if ((letterBoxedWidth % 2) != 0) letterBoxedWidth++;
    if ((letterBoxedHeight % 2) != 0) letterBoxedHeight++;
  }


  // ** cApplication

  cApplication::cApplication(int argc, const char* const* argv) :
    breathe::util::cApplication(argc, argv),

    pFont(nullptr),

    pGuiManager(nullptr),
    pGuiRenderer(nullptr)
  {
    settings.Load();
  }

  cApplication::~cApplication()
  {
    settings.Save();
  }

  #ifdef __WIN__
  void cApplication::AddMenu()
  {
    // Create our menu
    win32mm::cMenu menu;
    menu.CreateMenu();

    // Create our File popup menu
    win32mm::cPopupMenu popupFile;
    popupFile.AppendMenuItemWithShortcut(ID_MENU_FILE_OPEN_FOLDER, TEXT(LANGTAG_OPEN_FOLDER), KEY_COMBO_CONTROL('O'));
    popupFile.AppendMenuItemWithShortcut(ID_MENU_FILE_QUIT, TEXT(LANGTAG_QUIT), KEY_COMBO_CONTROL('W'));

    // Create our File popup menu
    win32mm::cPopupMenu popupEdit;
    popupEdit.AppendMenuItemWithShortcut(ID_MENU_EDIT_CUT, TEXT(LANGTAG_CUT), KEY_COMBO_CONTROL('C'));

    menu.AppendPopupMenu(popupFile, TEXT(LANGTAG_FILE));
    menu.AppendPopupMenu(popupEdit, TEXT(LANGTAG_EDIT));

    window.SetMenu(menu);
  }

  void cApplication::AddStatusBar()
  {
    // Create status bar
    window.CreateStatusBar(statusBar);

    // Create status bar "compartments" one width 150, other 300, then 400... last -1 means that it fills the rest of the window
    const int widths[] = { 150, 300, 400, 800, 810, -1 };
    statusBar.SetWidths(widths, countof(widths));

    statusBar.SetText(1, TEXT("Hello"));
    statusBar.SetText(2, TEXT("Goodbye"));
    statusBar.SetText(4, TEXT("1"));
    statusBar.SetText(5, TEXT("2"));

    statusBar.Resize();
  }
  #endif

  void cApplication::PlaySound(breathe::audio::cBufferRef pBuffer)
  {
    breathe::audio::cSourceRef pSource = pAudioManager->CreateSourceAttachedToScreen(pBuffer);
    assert(pSource);

    pSource->Play();
  }

  bool cApplication::_Create()
  {
    LOG<<"cApplication::_Create"<<std::endl;

    #ifdef __WIN__
    ASSERT(pWindow != nullptr);

    // Store the windows handle
    HWND hwndWindow = pWindow->GetWindowHandle();

    window.SetWindowHandle(hwndWindow);

    // Add our menu
    AddMenu();

    // Tell SDL that we want to process system events
    SDL_EventState(SDL_SYSWMEVENT, SDL_ENABLE);

    // Add our status bar
    AddStatusBar();

    /*class cEvent
    {
    public:
      virtual ~cEvent() {}

      virtual void EventFunction(const cApplication& data) { (void)data; }
    };

    class MyEventDerived : public cEvent
    {
    public:
      virtual void EventFunction(const cApplication& data) override { (void)data; }
    };

    win32mm::cWindow window;

    win32mm::cRunOnMainThread<cApplication, cEvent> runOnMainThread(*this);
    runOnMainThread.Create(window);

    MyEventDerived* pEvent = new MyEventDerived;
    runOnMainThread.PushEventToMainThread(pEvent);

    HWND hwnd = NULL;
    UINT uMsg = 0;
    WPARAM wParam = 0;
    LPARAM lParam = 0;
    if (uMsg > WM_USER) {
      runOnMainThread.ProcessEvents(hwnd, uMsg, wParam, lParam);
    }*/
    #endif

    assert(pContext != nullptr);
    assert(pContext->IsValid());

    assert(pGuiManager == nullptr);
    assert(pGuiRenderer == nullptr);

    // Setup our gui
    pGuiManager = new breathe::gui::cManager;
    pGuiRenderer = new breathe::gui::cRenderer(*pGuiManager, system, *pContext);

    _LoadResources();

    // Push our first state
    PushState(new cStateMenu(*this));

    return true;
  }

  void cApplication::_Destroy()
  {
    _DestroyResources();

    spitfire::SAFE_DELETE(pGuiRenderer);
    spitfire::SAFE_DELETE(pGuiManager);
  }

  bool cApplication::_LoadResources()
  {
    LOG<<"cApplication::_LoadResources"<<std::endl;

    assert(pGuiManager != nullptr);
    assert(pGuiRenderer != nullptr);

    pFont = pContext->CreateFont(TEXT("data/fonts/pricedown.ttf"), 32, TEXT("data/shaders/font.vert"), TEXT("data/shaders/font.frag"));
    assert(pFont != nullptr);
    assert(pFont->IsValid());

    cLetterBox letterBox(pContext->GetWidth(), pContext->GetHeight());

    pGuiRenderer->LoadResources(letterBox.letterBoxedWidth, letterBox.letterBoxedHeight);

    // Load the resources of all the states
    std::list<breathe::util::cState*>::iterator iter = states.begin();
    const std::list<breathe::util::cState*>::iterator iterEnd = states.end();
    while (iter != iterEnd) {
      cState* pState = static_cast<cState*>(*iter);
      if (pState != nullptr) pState->LoadResources();

      iter++;
    }

    return true;
  }

  void cApplication::_DestroyResources()
  {
    LOG<<"cApplication::_DestroyResources"<<std::endl;

    // Destroy the resources of all the states
    std::list<breathe::util::cState*>::iterator iter = states.begin();
    const std::list<breathe::util::cState*>::iterator iterEnd = states.end();
    while (iter != iterEnd) {
      cState* pState = static_cast<cState*>(*iter);
      if (pState != nullptr) pState->DestroyResources();

      iter++;
    }

    assert(pGuiManager != nullptr);
    assert(pGuiRenderer != nullptr);

    pGuiRenderer->DestroyResources();

    if (pFont != nullptr) {
      pContext->DestroyFont(pFont);
      pFont = nullptr;
    }
  }

  void cApplication::OnApplicationWindowEvent(const breathe::gui::cWindowEvent& event)
  {
    LOG<<"cApplication::OnApplicationWindowEvent"<<std::endl;
    if (event.IsAboutToResize()) {
      LOG<<"cApplication::OnApplicationWindowEvent About to resize"<<std::endl;
      statusBar.Resize();
    }
  }
}
