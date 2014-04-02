#ifndef DIESEL_APPLICATION_H
#define DIESEL_APPLICATION_H

// Standard headers
#include <stack>

// Spitfire headers
#include <spitfire/math/math.h>

// Breathe headers
#include <breathe/audio/audio.h>

#include <breathe/gui/cManager.h>
#include <breathe/gui/cRenderer.h>

#include <breathe/util/cApplication.h>

#ifdef __WIN__
// libwin32mm headers
#include <libwin32mm/window.h>
#include <libwin32mm/keys.h>
#endif

// Diesel headers
#include "settings.h"

namespace diesel
{
  #ifdef __WIN__
  // Langtags
  // TODO: Move these to dynamic langtags
  #ifdef __WIN__
  #define LANGTAG_QUIT "Exit"
  #else
  #define LANGTAG_QUIT "Quit"
  #endif

  #define LANGTAG_FILE "File"
  #define LANGTAG_OPEN_FOLDER "Open Folder"
  #define LANGTAG_EDIT "Edit"
  #define LANGTAG_CUT "Cut"
  #define LANGTAG_VIEW "View"
  #define LANGTAG_SINGLE_PHOTO_MODE "Single Photo Mode"

  // Menu IDs
  const int ID_MENU_FILE_OPEN_FOLDER = 10000;
  const int ID_MENU_FILE_QUIT = 10001;
  const int ID_MENU_EDIT_CUT = 10004;
  const int ID_MENU_VIEW_SINGLE_PHOTO_MODE = 10005;
  #endif

  // ** A simple class for calculating letter box dimensions

  class cLetterBox
  {
  public:
    cLetterBox(size_t width, size_t height);

    size_t desiredWidth;
    size_t desiredHeight;
    float fDesiredRatio;

    float fRatio;

    size_t letterBoxedWidth;
    size_t letterBoxedHeight;
  };


  class cState;

  // ** cApplication

  class cApplication : public breathe::util::cApplication
  {
  public:
    friend class cState;

    cApplication(int argc, const char* const* argv);
    ~cApplication();

    void PlaySound(breathe::audio::cBufferRef pBuffer);

  protected:
    cSettings settings;

  private:
    #ifdef __WIN__
    void AddMenu();
    void AddStatusBar();
    #endif
    
    void ResizeStatusBar();

    virtual bool _Create() override;
    virtual void _Destroy() override;

    virtual bool _LoadResources() override;
    virtual void _DestroyResources() override;

    virtual void OnApplicationWindowEvent(const breathe::gui::cWindowEvent& event) override;

    win32mm::cWindow window;
    win32mm::cStatusBar statusBar;

    // Text
    opengl::cFont* pFont;

    // Gui
    breathe::gui::cManager* pGuiManager;
    breathe::gui::cRenderer* pGuiRenderer;
  };
}

#endif // DIESEL_APPLICATION_H
