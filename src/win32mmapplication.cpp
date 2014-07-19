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
#include <libwin32mm/aboutdialog.h>
#include <libwin32mm/filebrowse.h>
#include <libwin32mm/notify.h>
#include <libwin32mm/progressdialog.h>

// Diesel headers
#include "win32mmapplication.h"
#include "win32mmsettingsdialog.h"

namespace diesel
{
  // ** cOpenGLContext

  cOpenGLContext::cOpenGLContext() :
    control(NULL),
    hDC(NULL),
    hRC(NULL),
    pContext(nullptr)
  {
  }

  cOpenGLContext::~cOpenGLContext()
  {
    std::cout<<"cOpenGLContext::~cOpenGLContext"<<std::endl;
    ASSERT(!IsValid());
  }

  bool cOpenGLContext::Create(HWND _control)
  {
    if (control != NULL) return true;

    control = _control;

    hDC = ::GetDC(control);
    ASSERT(hDC != NULL);

    const PIXELFORMATDESCRIPTOR pfd = {
      sizeof(PIXELFORMATDESCRIPTOR),
      1,                               // Version Number
      PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
      PFD_TYPE_RGBA,                   // Request RGBA Format
      16,                              // Select Color Depth
      0, 0, 0, 0, 0, 0,                // Color Bits Ignored
      0,                               // No Alpha bits
      0,                               // Shift Bit Ignored
      0,                               // No Accumulation Buffer
      0, 0, 0, 0,                      // Accumulation Bits Ignored
      16,                              // 16Bit Z-Buffer (Depth Buffer)
      0,                               // No Stencil Buffer
      0,                               // No Auxiliary Buffer
      PFD_MAIN_PLANE,                  // Main Drawing Layer
      0,                               // Reserved
      0, 0, 0                          // Layer Masks Ignored
    };

    const GLuint PixelFormat = ::ChoosePixelFormat(hDC, &pfd);
    ASSERT(PixelFormat != 0);

    const bool bIsSetPixelFormat = (::SetPixelFormat(hDC, PixelFormat, &pfd) == TRUE);
    ASSERT(bIsSetPixelFormat);

    // Create OpenGL rendering context
    hRC = wglCreateContext(hDC);
    ASSERT(hDC != NULL);

    const bool bIsSetCurrentContext = (wglMakeCurrent(hDC, hRC) == TRUE);
    ASSERT(bIsSetCurrentContext);

    const int iMajor = 3;
    const int iMinor = 3;
    if (gl3wInit()) {
      LOGERROR<<"cOpenGLContext::_SetWindowVideoMode Failed to initialize OpenGL"<<std::endl;
      return false;
    }
    if (!gl3wIsSupported(iMajor, iMinor)) {
      LOGERROR<<TEXT("cOpenGLContext::_SetWindowVideoMode OpenGL ")<<spitfire::string::ToString(iMajor)<<TEXT(".")<<spitfire::string::ToString(iMinor)<<TEXT(" not supported")<<std::endl;
      return false;
    }

    // Set up our default clearing, projection etc.
    glClearColor(0.0, 0.0, 0.0, 0.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    std::cout<<"OpenGL Version: "<<(const char*)glGetString(GL_VERSION)<<", OpenGL Vendor: "<<(const char*)glGetString(GL_VENDOR)<<", OpenGL Renderer: "<<(const char*)glGetString(GL_RENDERER)<<std::endl;


    // Set our resolution
    resolution.width = 500;
    resolution.height = 500;
    resolution.pixelFormat = opengl::PIXELFORMAT::R8G8B8A8;
    // TODO: Get the size of the control

    // Init libopenglmm
    pContext = system.CreateSharedContextForControl(control, resolution);

    return (bIsSetPixelFormat && bIsSetCurrentContext);
  }

  void cOpenGLContext::Destroy()
  {
    // Destroy our context
    if (pContext != nullptr) {
      system.DestroyContext(pContext);
      pContext = nullptr;
    }

    // Release OpenGL rendering context
    if (hRC != NULL) {
      if (!wglMakeCurrent(NULL, NULL)) {
        std::cout<<"Release of DC and RC failed"<<std::endl;
      }
      if (!wglDeleteContext(hRC)) {
        std::cout<<"Release of Rendering Context failed"<<std::endl;
      }
      hRC = NULL;
    }

    // Release Device context
    if (hDC != NULL) {
      if (ReleaseDC(control, hDC) != 1) {
        std::cout<<"Release of Device Context failed"<<std::endl;
      }
      hDC = NULL;
    }

    // Make sure we do not refer to this control in the future
    control = NULL;
  }

  bool cOpenGLContext::IsValid() const
  {
    return (
      (hDC != NULL) &&
      (hRC != NULL) &&
      ((pContext != nullptr) && pContext->IsValid())
    );
  }

  void cOpenGLContext::Resize(size_t width, size_t height)
  {
    LOG<<"cOpenGLContext::Resize ("<<width<<"x"<<height<<")"<<std::endl;

    // Update our resolution
    resolution.width = width;
    resolution.height = height;

    // Resize the context
    pContext->ResizeWindow(resolution);
  }

  void cOpenGLContext::Begin()
  {
    ASSERT(IsValid());

    bool bIsSetCurrentContext = (wglMakeCurrent(hDC, hRC) == TRUE);
    ASSERT(bIsSetCurrentContext);
  }

  void cOpenGLContext::End()
  {
    ASSERT(IsValid());

    ::SwapBuffers(hDC);
  }

  opengl::cContext& cOpenGLContext::GetContext()
  {
    return *pContext;
  }


  // ** cPhotoBrowserControl

  cPhotoBrowserControl::cPhotoBrowserControl() :
    pListener(nullptr)
  {
  }

  void cPhotoBrowserControl::Create(win32mm::cWindow& parent, int idControl, cPhotoBrowserControlListener& listener)
  {
    win32mm::cOpenGLControl::Create(parent, idControl);

    context.Create(GetHandle());

    pListener = &listener;
  }

  void cPhotoBrowserControl::Destroy()
  {
    context.Destroy();
  }

  void cPhotoBrowserControl::OnSize()
  {
    context.Resize(GetWidth(), GetHeight());
  }

  void cPhotoBrowserControl::OnPaint()
  {
    context.Begin();

    if (pListener != nullptr) pListener->OnPhotoBrowserControlPaint(context.GetContext());

    context.End();
  }


  void cMyListener::OnPhotoBrowserControlPaint(opengl::cContext& context)
  {
    static bool bIsRed = false;
    if (bIsRed) glClearColor(1.0f, 0.0f, 0.0f, 1.0f);
    else glClearColor(0.0f, 1.0f, 0.0f, 1.0f);
    bIsRed = !bIsRed;
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glTranslatef(3.0f,0.0f,0.0f);
    glBegin(GL_TRIANGLES);
    glVertex3f(0.0f,1.0f,0.0f);
    glVertex3f(-1.0f,-1.0f,0.0f);
    glVertex3f(1.0f,-1.0f,0.0f);
    glEnd();
  }


  // Langtags
  // TODO: Move these to dynamic langtags
  #ifdef __WIN__
  #define LANGTAG_QUIT "Exit"
  #else
  #define LANGTAG_QUIT "Quit"
  #endif

  #define LANGTAG_FILE "File"
  #define LANGTAG_OPEN_FOLDER "Open Folder"
  #define LANGTAG_SETTINGS "Settings"
  #define LANGTAG_EDIT "Edit"
  #define LANGTAG_CUT "Cut"
  #define LANGTAG_VIEW "View"
  #define LANGTAG_SINGLE_PHOTO_MODE "Single Photo Mode"
  #define LANGTAG_HELP "Help"
  #define LANGTAG_ABOUT "About"

  // Menu IDs
  const int ID_MENU_FILE_OPEN_FOLDER = 10000;
  const int ID_MENU_FILE_SETTINGS = 10001;
  const int ID_MENU_FILE_QUIT = 10002;
  const int ID_MENU_EDIT_CUT = 10004;
  const int ID_MENU_VIEW_SINGLE_PHOTO_MODE = 10005;
  const int ID_MENU_HELP_ABOUT = 10006;

  const int ID_CONTROL_OPENGL = 101;
  const int ID_CONTROL_PATH = 102;
  const int ID_CONTROL_UP = 103;
  const int ID_CONTROL_SHOW_FOLDER = 104;
  const int ID_CONTROL_SCROLLBAR = 105;

  cMainWindow::cMainWindow(cApplication& _application) :
    application(_application),
    settings(_application.settings)
  {
  }

  void cMainWindow::OnInit()
  {
    // Add our menu
    AddMenu();
    // Add our status bar
    AddStatusBar();

    // Initialise the taskbar
    taskBar.Init(*this);

    // Create our path combobox
    comboBoxPath.CreateComboBox(*this, ID_CONTROL_PATH);
    comboBoxPath.AddString(TEXT("a"));
    comboBoxPath.AddString(TEXT("c"));
    comboBoxPath.AddString(TEXT("b"));

    comboBoxPath.SetText(TEXT("Hello"));

    // Create our OpenGL control
    openGLControl.Create(*this, ID_CONTROL_OPENGL, listener);

    win32mm::cIcon iconUp;
    iconUp.LoadFromFile(TEXT("data/icons/windows/folder_up.ico"), 32);

    buttonPathUp.Create(*this, ID_CONTROL_UP, iconUp);

    win32mm::cIcon iconShowFolder;
    iconShowFolder.LoadFromFile(TEXT("data/icons/windows/folder_show.ico"), 32);

    buttonPathShowFolder.Create(*this, ID_CONTROL_SHOW_FOLDER, iconShowFolder);

    scrollBar.CreateVertical(*this, 101);
    scrollBar.SetRange(0, 200);
    scrollBar.SetPageSize(20);
    scrollBar.SetPosition(50);

    // Load the previous window position
    LoadWindowPosition();
  }

  void cMainWindow::LoadWindowPosition()
  {
    int iShowCmd = 0;
    int iX = 0;
    int iY = 0;
    int iWidth = 0;
    int iHeight = 0;
    iShowCmd = SW_SHOWNORMAL;
    iWidth = 1080;
    iHeight = 720;
    /*if (settings.GetWindowStateAndPosition(TEXT("Window"), TEXT("Show"), iShowCmd, iX, iY, iWidth, iHeight))*/ {
      // Get a handle to the monitor
      const POINT position = { iX, iY };
      HMONITOR hMonitor = ::MonitorFromPoint(position, MONITOR_DEFAULTTONEAREST);

      // Get the monitor info
      MONITORINFO monInfo;
      monInfo.cbSize = sizeof(MONITORINFO);
      if (::GetMonitorInfo(hMonitor, &monInfo) == 0) {
        LOG<<TEXT("cMainWindow::LoadWindowPosition GetMonitorInfo FAILED, returning")<<std::endl;
        return;
      }

      // Adjust for work area
      iX += monInfo.rcWork.left - monInfo.rcMonitor.left;
      iY += monInfo.rcWork.top  - monInfo.rcMonitor.top;
      // Ensure top left point is on screen
      const POINT point = { iX, iY };
      if (::PtInRect(&monInfo.rcWork, point) == FALSE ) {
        iX = monInfo.rcWork.left;
        iY = monInfo.rcWork.top;
      }

      ::SetWindowPos(hwndWindow, NULL, iX, iY, iWidth, iHeight, iShowCmd);
    }

    OnResize(iWidth, iHeight);
  }

  void cMainWindow::OnInitFinished()
  {
    openGLControl.Update();
  }

  void cMainWindow::OnDestroy()
  {
    SaveWindowPosition();

    openGLControl.Destroy();

    comboBoxPath.Destroy();
    scrollBar.Destroy();
  }

  bool cMainWindow::OnQuit()
  {
    return true;
  }

  void cMainWindow::SaveWindowPosition()
  {
    /*// Get the current window position
    WINDOWPLACEMENT placement;
    placement.length = sizeof(WINDOWPLACEMENT);
    ::GetWindowPlacement(hwndWindow, &placement);

    // Save the settings 
    settings.SetWindowStateAndPosition(TEXT("Window"), TEXT("Show"), placement.showCmd, placement.rcNormalPosition.left, placement.rcNormalPosition.right - placement.rcNormalPosition.left, placement.rcNormalPosition.top, placement.rcNormalPosition.bottom - placement.rcNormalPosition.top);*/
  }

  void cMainWindow::AddMenu()
  {
    // Create our menu
    win32mm::cMenu menu;
    menu.Create();

    // Create our File menu
    win32mm::cPopupMenu popupFile;
    popupFile.AppendMenuItemWithShortcut(ID_MENU_FILE_OPEN_FOLDER, TEXT(LANGTAG_OPEN_FOLDER), KEY_COMBO_CONTROL('O'));
    popupFile.AppendMenuItem(ID_MENU_FILE_SETTINGS, TEXT(LANGTAG_SETTINGS));
    popupFile.AppendMenuItemWithShortcut(ID_MENU_FILE_QUIT, TEXT(LANGTAG_QUIT), KEY_COMBO_CONTROL('W'));

    // Create our Edit menu
    win32mm::cPopupMenu popupEdit;
    popupEdit.AppendMenuItemWithShortcut(ID_MENU_EDIT_CUT, TEXT(LANGTAG_CUT), KEY_COMBO_CONTROL('C'));

    // Create our View menu
    win32mm::cPopupMenu popupView;
    popupView.AppendMenuItem(ID_MENU_VIEW_SINGLE_PHOTO_MODE, TEXT(LANGTAG_SINGLE_PHOTO_MODE));

    // Create our Help menu
    win32mm::cPopupMenu popupHelp;
    popupHelp.AppendMenuItem(ID_MENU_HELP_ABOUT, TEXT(LANGTAG_ABOUT));

    menu.AppendPopupMenu(popupFile, TEXT(LANGTAG_FILE));
    menu.AppendPopupMenu(popupEdit, TEXT(LANGTAG_EDIT));
    menu.AppendPopupMenu(popupView, TEXT(LANGTAG_VIEW));
    menu.AppendPopupMenu(popupHelp, TEXT(LANGTAG_HELP));

    SetMenu(menu);
  }

  void cMainWindow::AddStatusBar()
  {
    // Create status bar
    statusBar.Create(*this);

    // Create status bar "compartments" one width 150, other 300, then 400... last -1 means that it fills the rest of the window
    const int widths[] = { 150, 300, 400, 800, 810, -1 };
    statusBar.SetWidths(widths, countof(widths));

    statusBar.SetText(1, TEXT("Hello"));
    statusBar.SetText(2, TEXT("Goodbye"));
    statusBar.SetText(4, TEXT("1"));
    statusBar.SetText(5, TEXT("2"));
  }

  void cMainWindow::OnResizing(size_t width, size_t height)
  {
    statusBar.Resize();
  }

  void cMainWindow::OnResize(size_t width, size_t height)
  {
    statusBar.Resize();

    int iWindowWidth = int(width);
    int iWindowHeight = int(height);

    int iStatusBarWidth = 0;
    int iStatusBarHeight = 0;
    GetControlSize(statusBar.GetHandle(), iStatusBarWidth, iStatusBarHeight);

    const int iScrollBarWidth = GetScrollBarWidth();
    const int iScrollBarHeight = iWindowHeight - iStatusBarHeight;

    MoveControl(scrollBar.GetHandle(), iWindowWidth - iScrollBarWidth, 0, iScrollBarWidth, iScrollBarHeight);

    iWindowWidth -= iScrollBarWidth;

    const int iButtonHeight = GetButtonHeight();
    const int iButtonWidth = iButtonHeight;
    const int iButtonsTotalWidth = (2 * (iButtonWidth + GetSpacerWidth()));
    const int iPathWidth = iWindowWidth - (iButtonsTotalWidth + (2 * GetSpacerWidth()));
    const int iPathHeight = GetComboBoxHeight();

    int x = GetSpacerWidth();
    int y = GetSpacerHeight();
    MoveControl(comboBoxPath.GetHandle(), x, y, iPathWidth, iPathHeight);
    x += iPathWidth + GetSpacerWidth();

    MoveControl(buttonPathUp.GetHandle(), x, y, iButtonWidth, iButtonHeight);
    x += iButtonWidth + GetSpacerWidth();
    MoveControl(buttonPathShowFolder.GetHandle(), x, y, iButtonWidth, iButtonHeight);
    x += iButtonWidth + GetSpacerWidth();

    y += max(iPathHeight, iButtonHeight) + GetSpacerHeight();
    iWindowHeight -= max(iPathHeight, iButtonHeight) + (2 * GetSpacerHeight());

    MoveControl(openGLControl.GetHandle(), 0, y, iWindowWidth, iWindowHeight);

    // TODO: Move these somewhere else
    scrollBar.SetRange(0, 200);
    scrollBar.SetPosition(50);
  }

  bool cMainWindow::OnCommand(int idCommand)
  {
    switch (idCommand) {
      case ID_MENU_FILE_OPEN_FOLDER: {
        win32mm::cFolderDialog dialog;
        dialog.Run(*this);
        return true;
      }
      case ID_MENU_FILE_SETTINGS: {
        OpenSettingsDialog(settings, *this);
        return true;
      }
      case ID_MENU_FILE_QUIT: {
        //_OnStateQuitEvent();
        return true;
      }
      case ID_MENU_EDIT_CUT: {
        return true;
      }
      case ID_MENU_VIEW_SINGLE_PHOTO_MODE: {
        //bIsSinglePhotoMode = !bIsSinglePhotoMode;

        // TODO: UPDATE THE MENU ITEM CHECKBOX
        return true;
      }
      case ID_MENU_HELP_ABOUT: {
        win32mm::OpenAboutDialog(*this);
        return false;
      }
      case ID_CONTROL_PATH: {
        LOG<<"cStatePhotoBrowser::HandleCommand Path"<<std::endl;
        return true;
      }
      case ID_CONTROL_UP: {
        LOG<<"cStatePhotoBrowser::HandleCommand Up"<<std::endl;
        return true;
      }
    }

    return false;
  }



  // ** cApplication

  cApplication::cApplication(int argc, const char* const* argv) :
    spitfire::cConsoleApplication(argc, argv),

    mainWindow(*this),

    pFont(nullptr)
  {
    // Initialise libwin32mm
    win32mm::Init();

    settings.Load();
  }

  cApplication::~cApplication()
  {
    settings.Save();
  }

  void cApplication::_PrintHelp() const
  {
    LOG<<"Usage: "<<spitfire::string::ToUTF8(GetApplicationName())<<" [OPTIONS]"<<std::endl;
    LOG<<std::endl;
    LOG<<" -help, --help Display this help and exit"<<std::endl;
    LOG<<" -version, --version Display version information and exit"<<std::endl;
  }

  string_t cApplication::_GetVersion() const
  {
    ostringstream_t o;
    o<<GetApplicationName();
    o<<" "<<BUILD_APPLICATION_VERSION_STRING;
    return o.str();
  }

  bool cApplication::_Run()
  {
    const int iResult = mainWindow.Run();

    return (iResult == 0);
  }
}
